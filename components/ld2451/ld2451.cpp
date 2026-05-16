#include "ld2451.h"

#include <cstdio>
#include <cstring>

#include "ack_codec.h"
#include "ack_stream.h"
#include "config_state.h"
#include "target_publisher.h"
#include "control_entities.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t CMD_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t CMD_TAIL[] = {0x04, 0x03, 0x02, 0x01};
static const uint32_t POST_WRITE_READBACK_SETTLE_MS = 10;

static const char *detection_direction_option(uint8_t value) {
  switch (value) {
    case 0:
      return "away";
    case 1:
      return "approach";
    default:
      return "both";
  }
}

static std::string runtime_config_summary(const RuntimeConfig &config) {
  std::string summary;
  summary.reserve(160);
  summary += "max_distance=";
  summary += std::to_string(config.max_distance);
  summary += " detection_direction_raw=";
  summary += std::to_string(config.detection_direction);
  summary += " detection_direction_label=";
  summary += detection_direction_option(config.detection_direction);
  summary += " min_speed=";
  summary += std::to_string(config.min_speed);
  summary += " no_target_delay=";
  summary += std::to_string(config.no_target_delay);
  summary += " trigger_count=";
  summary += std::to_string(config.trigger_count);
  summary += " min_snr=";
  summary += std::to_string(config.min_snr);
  return summary;
}

static std::string format_bytes(const std::vector<uint8_t> &bytes) {
  if (bytes.empty()) {
    return "<empty>";
  }

  std::string out;
  out.reserve(bytes.size() * 3);
  char buf[4];
  for (size_t i = 0; i < bytes.size(); i++) {
    if (i > 0) {
      out += ' ';
    }
    std::snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned int>(bytes[i]));
    out += buf;
  }
  return out;
}

void LD2451Component::set_snr_threshold(int value) {
  if (value < 0) {
    value = 0;
  } else if (value > 64) {
    value = 64;
  }
  this->snr_threshold_ = static_cast<uint8_t>(value);
  this->set_min_snr(map_app_snr_to_native(this->snr_threshold_));
}

void LD2451Component::setup() {
  this->rx_buffer_.reserve(256);
  FirmwareVersionInfo fw{};
  if (this->read_firmware_version_(fw)) {
    const std::string fw_version = format_firmware_version(fw);
    ESP_LOGI(TAG, "Firmware: type=0x%04X version=%s (major=0x%04X minor=0x%08X)", fw.fw_type, fw_version.c_str(),
             fw.major, static_cast<unsigned int>(fw.minor));
  } else {
    ESP_LOGI(TAG, "Firmware: unavailable");
  }

  // Read device config once on boot as source of truth.
  RuntimeConfig device_config{};
  if (this->read_runtime_config_(device_config)) {
    normalize_distance_window(device_config);
    device_config.min_snr = coerce_native_min_snr(device_config.min_snr);
    this->desired_ = device_config;
    this->applied_ = device_config;
    ESP_LOGD(TAG, "Boot config sync: device is source of truth: %s",
             runtime_config_summary(this->desired_).c_str());
  } else {
    this->applied_ = this->desired_;
    ESP_LOGW(TAG, "Boot config sync failed; using compile-time defaults");
  }

  this->refresh_runtime_entities_();
  const IdleResetOutput reset = build_idle_reset_output();
  if (this->angle_sensor_ != nullptr) {
    this->angle_sensor_->publish_state(reset.angle);
  }
  if (this->distance_sensor_ != nullptr) {
    this->distance_sensor_->publish_state(reset.distance);
  }
  if (this->speed_sensor_ != nullptr) {
    this->speed_sensor_->publish_state(reset.speed);
  }
  if (this->snr_sensor_ != nullptr) {
    this->snr_sensor_->publish_state(reset.snr);
  }
  if (this->direction_text_sensor_ != nullptr) {
    this->direction_text_sensor_->publish_state(reset.direction);
  }
  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(false);
  }
  this->idle_published_ = true;
}

void LD2451Component::loop() {
  const uint32_t now = millis();
  size_t bytes_read = 0;
  while (this->available()) {
    this->rx_buffer_.push_back(this->read());
    bytes_read++;
  }

  if (bytes_read > 0) {
    if (now - this->last_rx_activity_log_ms_ > 5000) {
      ESP_LOGD(TAG, "RX activity: read=%u bytes, buffered=%u bytes", static_cast<unsigned int>(bytes_read),
               static_cast<unsigned int>(this->rx_buffer_.size()));
      this->last_rx_activity_log_ms_ = now;
    }
  }

  while (this->extract_frame_()) {
  }

  if (this->config_dirty_ && !this->config_in_flight_) {
    const uint32_t retry_ms = this->config_apply_failures_ > 4 ? 2000 : 250;
    if (now - this->last_config_attempt_ms_ >= retry_ms) {
      this->last_config_attempt_ms_ = now;
      this->config_in_flight_ = true;
      bool ok = this->apply_runtime_config_();
      this->config_in_flight_ = false;
      if (!ok) {
        this->config_apply_failures_++;
        ESP_LOGW(TAG, "Runtime config apply failed (attempt=%u); keeping previous applied settings",
                 static_cast<unsigned int>(this->config_apply_failures_));
        this->refresh_runtime_entities_();
      }
    }
  }


}

void LD2451Component::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2451:");
  ESP_LOGCONFIG(TAG,
                "  Runtime config: max_distance=%u min_distance=%u min_speed=%u detection_direction=%s no_target_delay=%u "
                "trigger_count=%u min_snr=%u snr_threshold=%u speed_correction=%.2f",
                this->desired_.max_distance, this->desired_.min_distance, this->desired_.min_speed,
                this->detection_direction_to_option_(this->desired_.detection_direction), this->desired_.no_target_delay,
                this->desired_.trigger_count, this->desired_.min_snr, this->snr_threshold_, this->desired_.speed_correction);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Vehicle Detected", this->vehicle_detected_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
  LOG_SENSOR("  ", "SNR", this->snr_sensor_);
  LOG_TEXT_SENSOR("  ", "Direction", this->direction_text_sensor_);
}

float LD2451Component::get_setup_priority() const { return setup_priority::DATA; }

void LD2451Component::set_max_distance(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.max_distance = static_cast<uint8_t>(value);
  normalize_distance_window(this->desired_);
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_min_distance(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.min_distance = static_cast<uint8_t>(value);
  if (this->desired_.min_distance > this->desired_.max_distance) {
    this->desired_.max_distance = this->desired_.min_distance;
  }
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_min_speed(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.min_speed = static_cast<uint8_t>(value);
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_detection_direction(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.detection_direction = static_cast<uint8_t>(value);
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_no_target_delay(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.no_target_delay = static_cast<uint8_t>(value);
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_trigger_count(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.trigger_count = static_cast<uint8_t>(value);
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_min_snr(int value) {
  const RuntimeConfig before = this->desired_;
  this->desired_.min_snr = coerce_native_min_snr(static_cast<uint8_t>(value));
  this->mark_config_dirty_if_changed_(before);
}

void LD2451Component::set_speed_correction(float value) { this->desired_.speed_correction = value; }

void LD2451Component::mark_config_dirty_if_changed_(const RuntimeConfig &before) {
  if (runtime_config_equal(before, this->desired_)) {
    return;
  }
  this->config_dirty_ = true;
}

bool LD2451Component::read_exact_(uint8_t *dest, size_t len, uint32_t timeout_ms) {
  const uint32_t start = millis();
  size_t offset = 0;
  while (offset < len) {
    if (this->available()) {
      dest[offset++] = this->read();
      continue;
    }
    if (millis() - start > timeout_ms) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool LD2451Component::send_command_wait_ack_(uint16_t command, const std::vector<uint8_t> &value, std::vector<uint8_t> *ret,
                                             uint32_t timeout_ms) {
  std::vector<uint8_t> payload;
  payload.reserve(2 + value.size());
  payload.push_back(static_cast<uint8_t>(command & 0xFF));
  payload.push_back(static_cast<uint8_t>((command >> 8) & 0xFF));
  payload.insert(payload.end(), value.begin(), value.end());

  const uint16_t payload_len = static_cast<uint16_t>(payload.size());

  this->write_array(CMD_HEADER, sizeof(CMD_HEADER));
  this->write_byte(static_cast<uint8_t>(payload_len & 0xFF));
  this->write_byte(static_cast<uint8_t>((payload_len >> 8) & 0xFF));
  for (auto byte : payload) {
    this->write_byte(byte);
  }
  this->write_array(CMD_TAIL, sizeof(CMD_TAIL));
  this->flush();

  const uint32_t start = millis();
  std::vector<uint8_t> rx_bytes;
  rx_bytes.reserve(96);
  while (true) {
    if (this->available()) {
      rx_bytes.push_back(this->read());
      const AckScanResult result = scan_for_matching_ack(rx_bytes, command, MAX_ACK_PAYLOAD_LEN, ret);
      if (result == AckScanResult::MATCHED) {
        return true;
      }
      continue;
    }
    if (millis() - start > timeout_ms) {
      return false;
    }
    delay(1);
  }

  return false;
}

bool LD2451Component::enter_config_mode_() {
  const std::vector<uint8_t> value = {0x01, 0x00};
  return this->send_command_wait_ack_(0x00FF, value);
}

bool LD2451Component::exit_config_mode_() {
  return this->send_command_wait_ack_(0x00FE, {}, nullptr, EXIT_CONFIG_ACK_TIMEOUT_MS);
}

bool LD2451Component::read_firmware_version_(FirmwareVersionInfo &out) {
  std::vector<uint8_t> ret;
  if (!this->send_command_wait_ack_(0x00A0, {}, &ret)) {
    return false;
  }
  return decode_firmware_version(ret, out);
}

bool LD2451Component::read_runtime_config_(RuntimeConfig &out) {
  out = this->desired_;
  if (!this->enter_config_mode_()) {
    ESP_LOGW(TAG, "Runtime config read failed at step: enter_config_mode");
    return false;
  }

  std::vector<uint8_t> target_ret;
  std::vector<uint8_t> sensitivity_ret;
  bool ok = true;

  if (!this->send_command_wait_ack_(0x0012, {}, &target_ret)) {
    ESP_LOGW(TAG, "Runtime config read failed at step: read_target_detection_params");
    ok = false;
  } else if (target_ret.size() < 4) {
    ESP_LOGW(TAG, "Runtime config read failed: target_detection_params response too short (%u)",
             static_cast<unsigned int>(target_ret.size()));
    ok = false;
  }

  if (ok && !this->send_command_wait_ack_(0x0013, {}, &sensitivity_ret)) {
    ESP_LOGW(TAG, "Runtime config read failed at step: read_sensitivity_params");
    ok = false;
  } else if (ok && sensitivity_ret.size() < 2) {
    ESP_LOGW(TAG, "Runtime config read failed: sensitivity_params response too short (%u)",
             static_cast<unsigned int>(sensitivity_ret.size()));
    ok = false;
  }

  const bool exit_ok = this->exit_config_mode_();
  if (!exit_ok) {
    ESP_LOGW(TAG, "Runtime config read: exit_config_mode failed (device likely auto-exited)");
  }

  if (!ok) {
    return false;
  }

  if (!exit_ok) {
    ESP_LOGW(TAG, "Runtime config read succeeded despite exit_config_mode failure");
  }

  out.max_distance = target_ret[0];
  out.detection_direction = target_ret[1];
  out.min_speed = target_ret[2];
  out.no_target_delay = target_ret[3];
  out.trigger_count = sensitivity_ret[0] == 0 ? 1 : sensitivity_ret[0];
  out.min_snr = sensitivity_ret[1];
  const std::string target_bytes = format_bytes(target_ret);
  const std::string sensitivity_bytes = format_bytes(sensitivity_ret);
  const std::string readback_summary = runtime_config_summary(out);
  ESP_LOGD(TAG, "Runtime config readback target_detection_params: bytes=[%s]", target_bytes.c_str());
  ESP_LOGD(TAG, "Runtime config readback sensitivity_params: bytes=[%s]", sensitivity_bytes.c_str());
  ESP_LOGD(TAG, "Runtime config readback decoded: %s", readback_summary.c_str());
  return true;
}

bool LD2451Component::write_target_detection_params_() {
  const std::vector<uint8_t> value = {
      this->desired_.max_distance,
      this->desired_.detection_direction,
      this->desired_.min_speed,
      this->desired_.no_target_delay,
  };
  const bool ok = this->send_command_wait_ack_(0x0002, value);
  const std::string value_bytes = format_bytes(value);
  const std::string desired_summary = runtime_config_summary(this->desired_);
  ESP_LOGD(TAG, "Runtime config write target_detection_params result: ok=%s bytes=[%s] desired={%s}",
           ok ? "true" : "false", value_bytes.c_str(), desired_summary.c_str());
  return ok;
}

bool LD2451Component::write_sensitivity_params_() {
  const std::vector<uint8_t> value = {
      this->desired_.trigger_count,
      this->desired_.min_snr,
      0x00,
      0x00,
  };
  const bool ok = this->send_command_wait_ack_(0x0003, value);
  const std::string value_bytes = format_bytes(value);
  const std::string desired_summary = runtime_config_summary(this->desired_);
  ESP_LOGD(TAG, "Runtime config write sensitivity_params result: ok=%s bytes=[%s] desired={%s}",
           ok ? "true" : "false", value_bytes.c_str(), desired_summary.c_str());
  return ok;
}

void LD2451Component::refresh_runtime_entities_() {
  if (this->max_distance_number_ != nullptr) {
    this->max_distance_number_->publish_state(this->desired_.max_distance);
  }
  if (this->min_distance_number_ != nullptr) {
    this->min_distance_number_->publish_state(this->desired_.min_distance);
  }
  if (this->min_speed_number_ != nullptr) {
    this->min_speed_number_->publish_state(this->desired_.min_speed);
  }
  if (this->no_target_delay_number_ != nullptr) {
    this->no_target_delay_number_->publish_state(this->desired_.no_target_delay);
  }
  if (this->trigger_count_number_ != nullptr) {
    this->trigger_count_number_->publish_state(this->desired_.trigger_count);
  }
  if (this->min_snr_number_ != nullptr) {
    this->min_snr_number_->publish_state(this->desired_.min_snr);
  }
  if (this->snr_threshold_number_ != nullptr) {
    this->snr_threshold_number_->publish_state(this->snr_threshold_);
  }
  if (this->speed_correction_number_ != nullptr) {
    this->speed_correction_number_->publish_state(this->desired_.speed_correction);
  }
  if (this->detection_direction_select_ != nullptr) {
    this->detection_direction_select_->publish_state(this->detection_direction_to_option_(this->desired_.detection_direction));
  }
}

uint8_t LD2451Component::detection_direction_from_option_(const std::string &value) {
  if (value == "away") {
    return 0;
  }
  if (value == "approach") {
    return 1;
  }
  return 2;
}

const char *LD2451Component::detection_direction_to_option_(uint8_t value) {
  switch (value) {
    case 0:
      return "away";
    case 1:
      return "approach";
    default:
      return "both";
  }
}

bool LD2451Component::apply_runtime_config_() {
  if (!this->enter_config_mode_()) {
    ESP_LOGW(TAG, "Runtime config apply failed at step: enter_config_mode");
    return false;
  }
  if (!this->write_target_detection_params_()) {
    ESP_LOGW(TAG, "Runtime config apply failed at step: write_target_detection_params");
    this->exit_config_mode_();
    return false;
  }
  if (!this->write_sensitivity_params_()) {
    ESP_LOGW(TAG, "Runtime config apply failed at step: write_sensitivity_params");
    this->exit_config_mode_();
    return false;
  }
  if (!this->exit_config_mode_()) {
    ESP_LOGW(TAG, "Runtime config apply failed at step: exit_config_mode");
    return false;
  }

  this->applied_ = this->desired_;
  this->config_dirty_ = false;
  this->config_apply_failures_ = 0;
  this->refresh_runtime_entities_();

  // Allow the device to settle after config-mode exit before opening a fresh readback session.
  delay(POST_WRITE_READBACK_SETTLE_MS);

  RuntimeConfig readback{};
  if (!this->read_runtime_config_(readback)) {
    ESP_LOGW(TAG, "Runtime config post-write readback failed");
    return true;
  }

  const std::string readback_summary = runtime_config_summary(readback);
  const std::string desired_summary = runtime_config_summary(this->desired_);
  const bool readback_matches = runtime_config_readback_fields_equal(this->desired_, readback);
  ESP_LOGD(TAG, "Runtime config post-write readback result: matches=%s desired={%s} readback={%s}",
           readback_matches ? "true" : "false", desired_summary.c_str(), readback_summary.c_str());
  ESP_LOGI(TAG, "Runtime config post-write readback: %s", readback_summary.c_str());
  if (!readback_matches) {
    ESP_LOGW(TAG,
             "Runtime config post-write mismatch (readback fields only): desired={%s} readback={%s}",
             desired_summary.c_str(),
             readback_summary.c_str());
  }
  return true;
}

bool LD2451Component::extract_frame_() {
  if (this->rx_buffer_.size() < 10) {
    return false;
  }

  size_t header_pos = this->rx_buffer_.size();
  for (size_t i = 0; i + 4 <= this->rx_buffer_.size(); i++) {
    if (this->rx_buffer_[i] == DATA_HEADER[0] && this->rx_buffer_[i + 1] == DATA_HEADER[1] &&
        this->rx_buffer_[i + 2] == DATA_HEADER[2] && this->rx_buffer_[i + 3] == DATA_HEADER[3]) {
      header_pos = i;
      break;
    }
  }

  if (header_pos == this->rx_buffer_.size()) {
    if (this->rx_buffer_.size() > 3) {
      this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.end() - 3);
    }
    return false;
  }

  if (header_pos > 0) {
    this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + static_cast<long>(header_pos));
  }

  if (this->rx_buffer_.size() < 10) {
    return false;
  }

  const uint16_t payload_len = static_cast<uint16_t>(this->rx_buffer_[4]) | (static_cast<uint16_t>(this->rx_buffer_[5]) << 8);
  const size_t frame_len = static_cast<size_t>(payload_len) + 10;
  if (this->rx_buffer_.size() < frame_len) {
    return false;
  }

  const size_t tail_pos = frame_len - 4;
  if (this->rx_buffer_[tail_pos] != DATA_TAIL[0] || this->rx_buffer_[tail_pos + 1] != DATA_TAIL[1] ||
      this->rx_buffer_[tail_pos + 2] != DATA_TAIL[2] || this->rx_buffer_[tail_pos + 3] != DATA_TAIL[3]) {
    this->rx_buffer_.erase(this->rx_buffer_.begin());
    return true;
  }

  std::vector<uint8_t> payload;
  payload.reserve(payload_len);
  payload.insert(payload.end(), this->rx_buffer_.begin() + 6, this->rx_buffer_.begin() + 6 + payload_len);

  if (payload_len < 2) {
    this->short_payload_frames_++;
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG, "Frame %u has short payload len=%u (short frames=%u)",
               static_cast<unsigned int>(this->parsed_frames_ + 1), static_cast<unsigned int>(payload_len),
               static_cast<unsigned int>(this->short_payload_frames_));
      this->last_empty_hint_ms_ = now;
    }
  }

  uint8_t target_count = 0;
  ParsedTarget target{};
  bool has_target = this->parse_payload_(payload, target_count, target);
  this->parsed_frames_++;

  if (!has_target) {
    this->empty_frames_++;
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG,
               "Live frame %u has no target payload (empty frames=%u). Detection may require larger movement; "
               "for pre-install checks, move decisively toward the sensor (e.g. walk/run toward it or use broad hand/body motion).",
               this->parsed_frames_, this->empty_frames_);
      this->last_empty_hint_ms_ = now;
    }
  } else {
    ESP_LOGD(TAG,
             "Frame %u: targets=%u angle=%ddeg dist=%um speed=%ukm/h dir_raw=0x%02X dir=%s snr=%u", this->parsed_frames_,
             target_count, target.angle, target.distance, target.speed, target.direction, direction_label(target.direction),
             target.snr);
  }

  this->publish_frame_(target_count, target, has_target);

  this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + static_cast<long>(frame_len));
  return true;
}

bool LD2451Component::parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, ParsedTarget &first_target) {
  if (payload.size() < 2) {
    return false;
  }

  target_count = payload[0];
  if (target_count == 0 || payload.size() < 7) {
    return false;
  }

  first_target.angle = static_cast<int>(payload[2]) - 0x80;
  first_target.distance = payload[3];
  first_target.direction = payload[4];
  first_target.speed = payload[5];
  first_target.snr = payload[6];
  return true;
}

void LD2451Component::publish_frame_(uint8_t target_count, const ParsedTarget &first_target, bool has_target) {
  if (this->target_count_sensor_ != nullptr) {
    this->target_count_sensor_->publish_state(target_count);
  }

  const uint32_t now = millis();
  const auto publish_idle_reset = [this]() {
    const IdleResetOutput reset = build_idle_reset_output();
    if (this->angle_sensor_ != nullptr) {
      this->angle_sensor_->publish_state(reset.angle);
    }
    if (this->distance_sensor_ != nullptr) {
      this->distance_sensor_->publish_state(reset.distance);
    }
    if (this->speed_sensor_ != nullptr) {
      this->speed_sensor_->publish_state(reset.speed);
    }
    if (this->snr_sensor_ != nullptr) {
      this->snr_sensor_->publish_state(reset.snr);
    }
    if (this->direction_text_sensor_ != nullptr) {
      this->direction_text_sensor_->publish_state(reset.direction);
    }
    if (this->vehicle_detected_binary_sensor_ != nullptr) {
      this->vehicle_detected_binary_sensor_->publish_state(false);
    }
    this->detection_active_ = false;
    this->idle_published_ = true;
  };

  const auto maybe_publish_idle_reset = [this, now, &publish_idle_reset]() {
    if (should_publish_idle_reset(this->detection_active_, this->idle_published_, now, this->last_detection_ms_,
                                  this->desired_.no_target_delay)) {
      publish_idle_reset();
    }
  };

  if (!has_target) {
    maybe_publish_idle_reset();
    return;
  }

  const TargetOutput output = compute_target_output(this->desired_, first_target);
  if (!output.publish) {
    ESP_LOGD(TAG, "Target filtered by distance window: %u (window=%u..%u)", first_target.distance,
             this->desired_.min_distance, this->desired_.max_distance);
    maybe_publish_idle_reset();
    return;
  }

  this->last_detection_ms_ = now;
  this->detection_active_ = true;
  this->idle_published_ = false;

  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(true);
  }

  if (this->angle_sensor_ != nullptr) {
    this->angle_sensor_->publish_state(first_target.angle);
  }
  if (this->distance_sensor_ != nullptr) {
    this->distance_sensor_->publish_state(first_target.distance);
  }
  if (this->speed_sensor_ != nullptr) {
    this->speed_sensor_->publish_state(output.corrected_speed);
  }
  if (this->snr_sensor_ != nullptr) {
    this->snr_sensor_->publish_state(first_target.snr);
  }
  if (this->direction_text_sensor_ != nullptr) {
    this->direction_text_sensor_->publish_state(direction_label(first_target.direction));
  }
}

}  // namespace ld2451
}  // namespace esphome
