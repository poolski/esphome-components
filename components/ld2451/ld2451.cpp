#include "ld2451.h"

#include <cmath>
#include <cstring>

#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t CMD_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t CMD_TAIL[] = {0x04, 0x03, 0x02, 0x01};

void LD2451MaxDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_max_distance(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_max_distance());
}

void LD2451MinDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_min_distance(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_min_distance());
}

void LD2451MinSpeedNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_min_speed(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_min_speed());
}

void LD2451NoTargetDelayNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_no_target_delay(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_no_target_delay());
}

void LD2451TriggerCountNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_trigger_count(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_trigger_count());
}

void LD2451MinSnrNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  int requested = static_cast<int>(lroundf(value));
  if (requested != 0 && requested < 3) {
    ESP_LOGW(TAG, "Coercing min_snr=%d to 0; allowed values are 0 or 3..8", requested);
    requested = 0;
  }
  this->parent_->set_min_snr(requested);
  this->publish_state(this->parent_->get_min_snr());
}

void LD2451SpeedCorrectionNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_speed_correction(value);
  this->publish_state(this->parent_->get_speed_correction());
}

void LD2451DetectionDirectionSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_detection_direction_option(value);
  this->publish_state(this->parent_->get_detection_direction_option());
}

void LD2451Component::setup() {
  this->rx_buffer_.reserve(256);
  this->applied_ = this->desired_;
  this->refresh_runtime_entities_();
}

void LD2451Component::loop() {
  size_t bytes_read = 0;
  while (this->available()) {
    this->rx_buffer_.push_back(this->read());
    bytes_read++;
  }

  if (bytes_read > 0) {
    const uint32_t now = millis();
    if (now - this->last_rx_activity_log_ms_ > 5000) {
      ESP_LOGD(TAG, "RX activity: read=%u bytes, buffered=%u bytes", static_cast<unsigned int>(bytes_read),
               static_cast<unsigned int>(this->rx_buffer_.size()));
      this->last_rx_activity_log_ms_ = now;
    }
  }

  while (this->extract_frame_()) {
  }

  if (this->config_dirty_ && !this->config_in_flight_) {
    const uint32_t now = millis();
    if (now - this->last_config_attempt_ms_ >= 250) {
      this->last_config_attempt_ms_ = now;
      this->config_in_flight_ = true;
      bool ok = this->apply_runtime_config_();
      this->config_in_flight_ = false;
      if (!ok) {
        ESP_LOGW(TAG, "Runtime config apply failed; keeping previous applied settings");
        this->refresh_runtime_entities_();
      }
    }
  }
}

void LD2451Component::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2451:");
  ESP_LOGCONFIG(TAG,
                "  Runtime config: max_distance=%u min_distance=%u min_speed=%u detection_direction=%s no_target_delay=%u "
                "trigger_count=%u min_snr=%u speed_correction=%.2f",
                this->desired_.max_distance, this->desired_.min_distance, this->desired_.min_speed,
                this->detection_direction_to_option_(this->desired_.detection_direction), this->desired_.no_target_delay,
                this->desired_.trigger_count, this->desired_.min_snr, this->desired_.speed_correction);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Alarm", this->alarm_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
  LOG_SENSOR("  ", "SNR", this->snr_sensor_);
  LOG_TEXT_SENSOR("  ", "Direction", this->direction_text_sensor_);
}

float LD2451Component::get_setup_priority() const { return setup_priority::DATA; }

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
  uint8_t rolling[4] = {0};
  size_t fill = 0;
  while (true) {
    if (this->available()) {
      uint8_t b = this->read();
      if (fill < 4) {
        rolling[fill++] = b;
      } else {
        rolling[0] = rolling[1];
        rolling[1] = rolling[2];
        rolling[2] = rolling[3];
        rolling[3] = b;
      }
      if (fill == 4 && std::memcmp(rolling, CMD_HEADER, sizeof(CMD_HEADER)) == 0) {
        break;
      }
      continue;
    }
    if (millis() - start > timeout_ms) {
      return false;
    }
    delay(1);
  }

  uint8_t len_bytes[2];
  if (!this->read_exact_(len_bytes, 2, timeout_ms)) {
    return false;
  }
  const uint16_t ack_len = static_cast<uint16_t>(len_bytes[0]) | (static_cast<uint16_t>(len_bytes[1]) << 8);
  if (ack_len < 4) {
    return false;
  }
  if (ack_len > MAX_ACK_PAYLOAD_LEN) {
    ESP_LOGW(TAG, "Rejecting ACK with unexpected length %u", ack_len);
    return false;
  }

  std::vector<uint8_t> ack_payload(ack_len);
  if (!this->read_exact_(ack_payload.data(), ack_payload.size(), timeout_ms)) {
    return false;
  }

  uint8_t tail[4];
  if (!this->read_exact_(tail, sizeof(tail), timeout_ms)) {
    return false;
  }
  if (std::memcmp(tail, CMD_TAIL, sizeof(CMD_TAIL)) != 0) {
    return false;
  }

  const uint16_t ack_cmd = static_cast<uint16_t>(ack_payload[0]) | (static_cast<uint16_t>(ack_payload[1]) << 8);
  const uint16_t status = static_cast<uint16_t>(ack_payload[2]) | (static_cast<uint16_t>(ack_payload[3]) << 8);
  if (ack_cmd != static_cast<uint16_t>(command | 0x0100) || status != 0) {
    return false;
  }

  if (ret != nullptr) {
    ret->assign(ack_payload.begin() + 4, ack_payload.end());
  }
  return true;
}

bool LD2451Component::enter_config_mode_() {
  const std::vector<uint8_t> value = {0x01, 0x00};
  return this->send_command_wait_ack_(0x00FF, value);
}

bool LD2451Component::exit_config_mode_() { return this->send_command_wait_ack_(0x00FE, {}); }

bool LD2451Component::write_target_detection_params_() {
  const std::vector<uint8_t> value = {
      this->desired_.max_distance,
      this->desired_.detection_direction,
      this->desired_.min_speed,
      this->desired_.no_target_delay,
  };
  return this->send_command_wait_ack_(0x0002, value);
}

bool LD2451Component::write_sensitivity_params_() {
  const std::vector<uint8_t> value = {
      this->desired_.trigger_count,
      this->desired_.min_snr,
      0x00,
      0x00,
  };
  return this->send_command_wait_ack_(0x0003, value);
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
    return false;
  }
  if (!this->write_target_detection_params_()) {
    this->exit_config_mode_();
    return false;
  }
  if (!this->write_sensitivity_params_()) {
    this->exit_config_mode_();
    return false;
  }
  if (!this->exit_config_mode_()) {
    return false;
  }

  this->applied_ = this->desired_;
  this->config_dirty_ = false;
  this->refresh_runtime_entities_();
  ESP_LOGD(TAG,
            "Applied runtime config: max_distance=%u min_speed=%u direction=%u no_target_delay=%u trigger_count=%u "
           "min_snr=%u",
           this->applied_.max_distance, this->applied_.min_speed, this->applied_.detection_direction,
           this->applied_.no_target_delay, this->applied_.trigger_count, this->applied_.min_snr);
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
  bool alarm = false;
  ParsedTarget target{};
  bool has_target = this->parse_payload_(payload, target_count, alarm, target);
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
             "Frame %u: targets=%u alarm=%s angle=%ddeg dist=%um speed=%ukm/h dir_raw=0x%02X dir=%s snr=%u",
             this->parsed_frames_, target_count, alarm ? "ON" : "OFF", target.angle, target.distance, target.speed,
             target.direction, target.direction == 0x01 ? "approaching" : "moving_away", target.snr);
  }

  this->publish_frame_(target_count, alarm, target, has_target);

  this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + static_cast<long>(frame_len));
  return true;
}

bool LD2451Component::parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                                     ParsedTarget &first_target) {
  if (payload.size() < 2) {
    return false;
  }

  target_count = payload[0];
  alarm = payload[1] != 0;
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

void LD2451Component::publish_frame_(uint8_t target_count, bool alarm, const ParsedTarget &first_target, bool has_target) {
  if (this->target_count_sensor_ != nullptr) {
    this->target_count_sensor_->publish_state(target_count);
  }
  if (this->alarm_binary_sensor_ != nullptr) {
    this->alarm_binary_sensor_->publish_state(alarm);
  }

  if (!has_target) {
    return;
  }

  if (first_target.distance < this->desired_.min_distance || first_target.distance > this->desired_.max_distance) {
    ESP_LOGD(TAG, "Target filtered by distance window: %u (window=%u..%u)", first_target.distance,
             this->desired_.min_distance, this->desired_.max_distance);
    return;
  }

  const float corrected_speed = static_cast<float>(first_target.speed) * this->desired_.speed_correction;

  if (this->angle_sensor_ != nullptr) {
    this->angle_sensor_->publish_state(first_target.angle);
  }
  if (this->distance_sensor_ != nullptr) {
    this->distance_sensor_->publish_state(first_target.distance);
  }
  if (this->speed_sensor_ != nullptr) {
    this->speed_sensor_->publish_state(corrected_speed);
  }
  if (this->snr_sensor_ != nullptr) {
    this->snr_sensor_->publish_state(first_target.snr);
  }
  if (this->direction_text_sensor_ != nullptr) {
    // Observed on verified LD2451 stream: 0x01 = approaching, 0x00 = moving away.
    this->direction_text_sensor_->publish_state(first_target.direction == 0x01 ? "approaching" : "moving_away");
  }
}

}  // namespace ld2451
}  // namespace esphome
