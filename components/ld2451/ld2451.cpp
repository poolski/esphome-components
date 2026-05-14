#include "ld2451.h"

#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};

void LD2451Component::setup() { this->rx_buffer_.reserve(256); }

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
}

void LD2451Component::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2451:");
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Alarm", this->alarm_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
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

  if (this->angle_sensor_ != nullptr) {
    this->angle_sensor_->publish_state(first_target.angle);
  }
  if (this->distance_sensor_ != nullptr) {
    this->distance_sensor_->publish_state(first_target.distance);
  }
  if (this->speed_sensor_ != nullptr) {
    this->speed_sensor_->publish_state(first_target.speed);
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
