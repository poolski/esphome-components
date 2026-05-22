#include "ld2451.h"

#include "config_state.h"
#include "target_publisher.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
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

  bool parsed_frame = false;
  while (this->extract_frame_()) {
    parsed_frame = true;
  }
  if (bytes_read > 0 && !parsed_frame) {
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
  ESP_LOGCONFIG(TAG, "  Speed Correction:    %.2fx (software only)", this->desired_.speed_correction);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Vehicle Detected", this->vehicle_detected_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
  LOG_SENSOR("  ", "Speed MPH", this->speed_mph_sensor_);
  LOG_SENSOR("  ", "SNR", this->snr_sensor_);
  LOG_TEXT_SENSOR("  ", "Direction", this->direction_text_sensor_);
}

float LD2451Component::get_setup_priority() const { return setup_priority::DATA; }

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
  ParsedTarget target{};
  bool has_target = this->parse_payload_(payload, target_count, target);

  if (!has_target) {
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG, "No target payload received. Sensor telemetry is active.");
      this->last_empty_hint_ms_ = now;
    }
  } else {
    ESP_LOGD(TAG, "Parsed telemetry: targets=%u angle=%ddeg dist=%um speed=%ukm/h dir_raw=0x%02X dir=%s snr=%u",
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
  if (target_count == 0) {
    return false;
  }

  const size_t required_size = 2 + static_cast<size_t>(target_count) * 5;
  if (payload.size() < required_size) {
    return false;
  }

  const bool alarm = (payload[1] == 0x01);
  std::vector<ParsedTarget> candidates;
  candidates.reserve(target_count);
  for (size_t i = 0; i < target_count; i++) {
    const size_t offset = 2 + i * 5;
    ParsedTarget candidate{};
    candidate.angle = static_cast<int>(payload[offset]) - 0x80;
    candidate.distance = payload[offset + 1];
    candidate.direction = payload[offset + 2];
    candidate.speed = payload[offset + 3];
    candidate.snr = payload[offset + 4];
    candidates.push_back(candidate);
  }

  if (!select_nearest_qualifying_target(this->desired_, candidates, first_target)) {
    return false;
  }
  first_target.alarm = alarm;
  return true;
}

void LD2451Component::publish_frame_(uint8_t target_count, const ParsedTarget &first_target, bool has_target) {
  if (this->target_count_sensor_ != nullptr) {
    const float new_count = static_cast<float>(target_count);
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

  if (!has_target) {
    maybe_publish_idle_reset();
    return;
  }

  const TargetOutput output = compute_target_output(this->desired_, first_target);
  if (!output.publish) {
    ESP_LOGD(TAG, "Target filtered: distance %u < min_distance %u", first_target.distance,
             this->desired_.min_distance);
    maybe_publish_idle_reset();
    return;
  }

  this->last_detection_ms_ = now;
  this->detection_active_ = true;
  this->idle_published_ = false;

  // vehicle_detected fires only when the device alarm flag is set (trigger_count met).
  // Sensor values (distance, speed, etc.) are published for any qualifying target.
  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(output.alarm);
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
  if (this->speed_mph_sensor_ != nullptr) {
    this->speed_mph_sensor_->publish_state(output.corrected_speed_mph);
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
