#include "ld2451.h"

#include "config_state.h"
#include "target_publisher.h"
#include <limits>
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t CONFIG_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t CONFIG_TAIL[]   = {0x04, 0x03, 0x02, 0x01};

static size_t count_present_targets(const std::array<LiveTargetOutput, kLiveTargetSlotCount> &outputs);

void LD2451Component::set_live_target_angle_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].angle = sensor;
  }
}

void LD2451Component::set_live_target_x_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].x = sensor;
  }
}

void LD2451Component::set_live_target_y_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].y = sensor;
  }
}

void LD2451Component::set_live_target_distance_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].distance = sensor;
  }
}

void LD2451Component::set_live_target_speed_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].speed = sensor;
  }
}

void LD2451Component::set_live_target_speed_mph_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].speed_mph = sensor;
  }
}

void LD2451Component::set_live_target_snr_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].snr = sensor;
  }
}

void LD2451Component::set_live_target_direction_text_sensor(uint8_t slot, text_sensor::TextSensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].direction = sensor;
  }
}

void LD2451Component::setup() {
  this->rx_buffer_.reserve(256);
  ESP_LOGI(TAG, "Runtime configuration via ESPHome is disabled");
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

  bool any_frame = false;
  bool keep_going = true;
  while (keep_going) {
    bool frame_produced = false;
    keep_going = this->extract_frame_(frame_produced);
    any_frame |= frame_produced;
  }
  if (bytes_read > 0 && !any_frame) {
    if (now - this->last_rx_activity_log_ms_ > 5000) {
      ESP_LOGD(TAG, "RX activity: read=%u bytes, buffered=%u bytes", static_cast<unsigned int>(bytes_read),
               static_cast<unsigned int>(this->rx_buffer_.size()));
      this->last_rx_activity_log_ms_ = now;
    }
  }
}

void LD2451Component::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2451:");
  ESP_LOGCONFIG(TAG, "  Runtime config:      disabled in ESPHome");
  ESP_LOGCONFIG(TAG, "  Min Distance:        %u m (software filter)", this->desired_.min_distance);
  ESP_LOGCONFIG(TAG, "  Speed Publish Min SNR: %u (software filter)", this->desired_.speed_publish_min_snr);
  ESP_LOGCONFIG(TAG, "  Speed Publish Max Angle: %u deg (software filter)",
                this->desired_.speed_publish_max_abs_angle);
  ESP_LOGCONFIG(TAG, "  Speed Correction:    %.2fx (software only)", this->desired_.speed_correction);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Vehicle Detected", this->vehicle_detected_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
  LOG_SENSOR("  ", "Speed MPH", this->speed_mph_sensor_);
  LOG_SENSOR("  ", "SNR", this->snr_sensor_);
  LOG_TEXT_SENSOR("  ", "Direction", this->direction_text_sensor_);
  for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
    ESP_LOGCONFIG(TAG, "  Target %u:", static_cast<unsigned int>(i + 1));
    LOG_SENSOR("    ", "X", this->live_target_sensors_[i].x);
    LOG_SENSOR("    ", "Y", this->live_target_sensors_[i].y);
    LOG_SENSOR("    ", "Angle", this->live_target_sensors_[i].angle);
    LOG_SENSOR("    ", "Distance", this->live_target_sensors_[i].distance);
    LOG_SENSOR("    ", "Speed", this->live_target_sensors_[i].speed);
    LOG_SENSOR("    ", "Speed MPH", this->live_target_sensors_[i].speed_mph);
    LOG_SENSOR("    ", "SNR", this->live_target_sensors_[i].snr);
    LOG_TEXT_SENSOR("    ", "Direction", this->live_target_sensors_[i].direction);
  }
}

float LD2451Component::get_setup_priority() const { return setup_priority::DATA; }

bool LD2451Component::extract_frame_(bool &frame_produced) {
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
    // No data header found. Check for a config/ACK frame and skip it cleanly.
    size_t config_pos = this->rx_buffer_.size();
    for (size_t i = 0; i + 4 <= this->rx_buffer_.size(); i++) {
      if (this->rx_buffer_[i] == CONFIG_HEADER[0] && this->rx_buffer_[i + 1] == CONFIG_HEADER[1] &&
          this->rx_buffer_[i + 2] == CONFIG_HEADER[2] && this->rx_buffer_[i + 3] == CONFIG_HEADER[3]) {
        config_pos = i;
        break;
      }
    }
    if (config_pos < this->rx_buffer_.size()) {
      // Found a config header. Need at least 10 bytes (header + len + tail) to read the length.
      const size_t bytes_from_header = this->rx_buffer_.size() - config_pos;
      if (bytes_from_header >= 10) {
        const uint16_t payload_len = static_cast<uint16_t>(this->rx_buffer_[config_pos + 4]) |
                                     (static_cast<uint16_t>(this->rx_buffer_[config_pos + 5]) << 8);
        const size_t frame_len = static_cast<size_t>(payload_len) + 10;
        if (bytes_from_header >= frame_len) {
          ESP_LOGD(TAG, "Config/ACK frame skipped (payload_len=%u)", payload_len);
          this->rx_buffer_.erase(this->rx_buffer_.begin(),
                                 this->rx_buffer_.begin() + static_cast<long>(config_pos + frame_len));
          return true;
        }
        // Not enough bytes yet for the full config frame; leave the buffer untouched.
        return false;
      }
      // Not enough bytes yet to read the length; leave the buffer untouched.
      return false;
    }
    // No config header either — discard all but the last 3 bytes (may be a partial header).
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

  if (payload_len == 0) {
    ESP_LOGD(TAG, "Heartbeat frame received: valid frame with no target readings");
  } else if (payload_len < 2) {
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG, "Short payload received: len=%u", static_cast<unsigned int>(payload_len));
      this->last_empty_hint_ms_ = now;
    }
  }

  uint8_t target_count = 0;
  bool alarm = false;
  std::vector<ParsedTarget> targets;
  bool has_targets = this->parse_payload_(payload, target_count, alarm, targets);
  const auto live_targets = build_live_target_outputs(this->desired_, targets);
  const size_t confident_target_count = count_present_targets(live_targets);

  if (!has_targets) {
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG, "No target payload received. Sensor telemetry is active.");
      this->last_empty_hint_ms_ = now;
    }
  } else if (confident_target_count == 0 && !targets.empty()) {
    const std::string reason = confidence_filter_reason(this->desired_, targets.front());
    ESP_LOGD(TAG, "Ignored target due to low confidence. [%s]", reason.c_str());
  } else {
    const LiveTargetOutput *first_present = nullptr;
    for (const auto &candidate : live_targets) {
      if (candidate.present) {
        first_present = &candidate;
        break;
      }
    }
    if (first_present != nullptr) {
      ESP_LOGD(TAG,
               "Parsed telemetry: targets=%u first_angle=%ddeg first_dist=%um first_speed=%ukm/h first_dir_raw=0x%02X first_dir=%s first_snr=%u",
               target_count, first_present->target.angle, first_present->target.distance, first_present->target.speed,
               first_present->target.direction, direction_label(first_present->target.direction),
               first_present->target.snr);
    }
  }

  this->publish_frame_(target_count, targets, alarm, has_targets);

  this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + static_cast<long>(frame_len));
  frame_produced = true;
  return true;
}

bool LD2451Component::parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                                     std::vector<ParsedTarget> &targets) {
  if (payload.size() < 2) {
    return false;
  }

  target_count = payload[0];
  if (target_count == 0) {
    return false;
  }

  const size_t required_size = 2 + static_cast<size_t>(target_count) * 5;
  if (payload.size() < required_size) {
    return false;
  }

  alarm = (payload[1] == 0x01);
  targets.clear();
  targets.reserve(target_count);
  for (size_t i = 0; i < target_count; i++) {
    const size_t offset = 2 + i * 5;
    ParsedTarget candidate{};
    candidate.angle = static_cast<int>(payload[offset]) - 0x80;
    candidate.distance = payload[offset + 1];
    candidate.direction = payload[offset + 2];
    candidate.speed = payload[offset + 3];
    candidate.snr = payload[offset + 4];
    targets.push_back(candidate);
  }

  return true;
}

void LD2451Component::clear_live_target_slot_(uint8_t slot) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  const auto &sensors = this->live_target_sensors_[slot];
  if (sensors.x != nullptr) {
    sensors.x->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.y != nullptr) {
    sensors.y->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.angle != nullptr) {
    sensors.angle->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.distance != nullptr) {
    sensors.distance->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.speed != nullptr) {
    sensors.speed->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.speed_mph != nullptr) {
    sensors.speed_mph->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.snr != nullptr) {
    sensors.snr->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.direction != nullptr) {
    sensors.direction->publish_state("None");
  }
}

static size_t count_present_targets(const std::array<LiveTargetOutput, kLiveTargetSlotCount> &outputs) {
  size_t count = 0;
  for (const auto &output : outputs) {
    if (output.present) {
      count++;
    }
  }
  return count;
}

void LD2451Component::publish_live_target_slot_(uint8_t slot, const LiveTargetOutput &output) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  if (!output.present) {
    this->clear_live_target_slot_(slot);
    return;
  }

  const auto &sensors = this->live_target_sensors_[slot];
  if (sensors.x != nullptr) {
    sensors.x->publish_state(output.x);
  }
  if (sensors.y != nullptr) {
    sensors.y->publish_state(output.y);
  }
  if (sensors.angle != nullptr) {
    sensors.angle->publish_state(output.target.angle);
  }
  if (sensors.distance != nullptr) {
    sensors.distance->publish_state(output.target.distance);
  }
  if (sensors.speed != nullptr) {
    sensors.speed->publish_state(output.corrected_speed);
  }
  if (sensors.speed_mph != nullptr) {
    sensors.speed_mph->publish_state(output.corrected_speed_mph);
  }
  if (sensors.snr != nullptr) {
    sensors.snr->publish_state(output.target.snr);
  }
  if (sensors.direction != nullptr) {
    sensors.direction->publish_state(direction_label(output.target.direction));
  }
}

void LD2451Component::publish_frame_(uint8_t target_count, const std::vector<ParsedTarget> &targets, bool alarm,
                                     bool has_targets) {
  const auto live_targets = build_live_target_outputs(this->desired_, targets);
  const size_t confident_target_count = count_present_targets(live_targets);

  if (this->target_count_sensor_ != nullptr) {
    const float new_count = static_cast<float>(confident_target_count);
    if (!this->target_count_sensor_->has_state() || this->target_count_sensor_->state != new_count) {
      this->target_count_sensor_->publish_state(new_count);
    }
  }

  const uint32_t now = millis();
  const auto publish_idle_reset = [this]() {
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

  if (!has_targets || confident_target_count == 0) {
    for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
      this->clear_live_target_slot_(i);
    }
    maybe_publish_idle_reset();
    return;
  }

  for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
    this->publish_live_target_slot_(i, live_targets[i]);
  }

  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(alarm);
  }

  this->last_detection_ms_ = now;
  this->detection_active_ = true;
  this->idle_published_ = false;

  if (targets.size() > kLiveTargetSlotCount) {
    ESP_LOGD(TAG, "Frame contains %u targets; exposing first %u live slots",
             static_cast<unsigned int>(targets.size()), static_cast<unsigned int>(kLiveTargetSlotCount));
  }

  ParsedTarget nearest_target{};
  const bool has_nearest_target = select_nearest_qualifying_target(this->desired_, targets, nearest_target);
  if (!has_nearest_target) {
    ESP_LOGD(TAG, "No qualifying target found for nearest-target entities (min_distance=%u)",
             this->desired_.min_distance);
  } else {
    const TargetOutput output = compute_target_output(this->desired_, nearest_target);
    // vehicle_detected fires only when the device alarm flag is set (trigger_count met).
    // Nearest target values are published for the closest qualifying target.
    if (this->vehicle_detected_binary_sensor_ != nullptr) {
      this->vehicle_detected_binary_sensor_->publish_state(alarm);
    }

    if (this->angle_sensor_ != nullptr) {
      this->angle_sensor_->publish_state(nearest_target.angle);
    }
    if (this->distance_sensor_ != nullptr) {
      this->distance_sensor_->publish_state(nearest_target.distance);
    }
    if (this->speed_sensor_ != nullptr) {
      this->speed_sensor_->publish_state(output.corrected_speed);
    }
    if (this->speed_mph_sensor_ != nullptr) {
      this->speed_mph_sensor_->publish_state(output.corrected_speed_mph);
    }
    if (this->snr_sensor_ != nullptr) {
      this->snr_sensor_->publish_state(nearest_target.snr);
    }
    if (this->direction_text_sensor_ != nullptr) {
      this->direction_text_sensor_->publish_state(direction_label(nearest_target.direction));
    }
  }

  if (confident_target_count < target_count) {
    ESP_LOGD(TAG, "Filtered %u low-confidence targets from frame", static_cast<unsigned int>(target_count - confident_target_count));
  }
}

}  // namespace ld2451
}  // namespace esphome
