#include "ld2451.h"

#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static const uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t CMD_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t CMD_TAIL[] = {0x04, 0x03, 0x02, 0x01};

static const uint8_t CMD_ENTER_CONFIG = 0xFF;
static const uint8_t CMD_EXIT_CONFIG = 0xFE;
static const uint8_t CMD_SET_BLUETOOTH = 0xA4;

void LD2451Component::setup() {
  this->rx_buffer_.reserve(256);

  if (this->disable_bluetooth_on_boot_) {
    const bool ok = this->set_bluetooth_enabled(false);
    if (!ok) {
      ESP_LOGW(TAG, "Failed to disable bluetooth on boot");
    }
  }

  if (this->bluetooth_switch_ != nullptr) {
    this->bluetooth_switch_->publish_state(this->bluetooth_enabled_state_);
  }
}

void LD2451Component::loop() {
  while (this->available()) {
    this->rx_buffer_.push_back(this->read());
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

bool LD2451Component::send_command_frame_(uint8_t cmd, const uint8_t *payload, size_t payload_len) {
  const uint16_t body_len = static_cast<uint16_t>(payload_len + 2);
  this->write_array(CMD_HEADER, sizeof(CMD_HEADER));
  this->write_byte(static_cast<uint8_t>(body_len & 0xFF));
  this->write_byte(static_cast<uint8_t>((body_len >> 8) & 0xFF));
  this->write_byte(cmd);
  this->write_byte(0x00);
  if (payload != nullptr && payload_len > 0) {
    this->write_array(payload, payload_len);
  }
  this->write_array(CMD_TAIL, sizeof(CMD_TAIL));
  this->flush();
  return true;
}

bool LD2451Component::read_ack_frame_(uint8_t expected_cmd, uint16_t timeout_ms) {
  std::vector<uint8_t> ack_buf;
  ack_buf.reserve(64);
  const uint32_t start = millis();

  while (millis() - start < timeout_ms) {
    while (this->available()) {
      ack_buf.push_back(this->read());
    }

    while (ack_buf.size() >= 10) {
      size_t header_pos = ack_buf.size();
      for (size_t i = 0; i + sizeof(CMD_HEADER) <= ack_buf.size(); i++) {
        if (ack_buf[i] == CMD_HEADER[0] && ack_buf[i + 1] == CMD_HEADER[1] && ack_buf[i + 2] == CMD_HEADER[2] &&
            ack_buf[i + 3] == CMD_HEADER[3]) {
          header_pos = i;
          break;
        }
      }

      if (header_pos == ack_buf.size()) {
        if (ack_buf.size() > 3) {
          ack_buf.erase(ack_buf.begin(), ack_buf.end() - 3);
        }
        break;
      }

      if (header_pos > 0) {
        ack_buf.erase(ack_buf.begin(), ack_buf.begin() + static_cast<long>(header_pos));
      }

      if (ack_buf.size() < 10) {
        break;
      }

      const uint16_t payload_len = static_cast<uint16_t>(ack_buf[4]) | (static_cast<uint16_t>(ack_buf[5]) << 8);
      const size_t frame_len = static_cast<size_t>(payload_len) + 10;
      if (ack_buf.size() < frame_len) {
        break;
      }

      const size_t tail_pos = frame_len - 4;
      if (ack_buf[tail_pos] != CMD_TAIL[0] || ack_buf[tail_pos + 1] != CMD_TAIL[1] || ack_buf[tail_pos + 2] != CMD_TAIL[2] ||
          ack_buf[tail_pos + 3] != CMD_TAIL[3]) {
        ack_buf.erase(ack_buf.begin());
        continue;
      }

      if (payload_len >= 4 && ack_buf[6] == expected_cmd && ack_buf[7] == 0x01 && ack_buf[8] == 0x00 && ack_buf[9] == 0x00) {
        return true;
      }

      ESP_LOGW(TAG, "Unexpected ACK payload for cmd 0x%02X", expected_cmd);
      return false;
    }

    delay(1);
  }

  ESP_LOGW(TAG, "Timeout waiting for ACK cmd 0x%02X", expected_cmd);
  return false;
}

bool LD2451Component::enter_config_() {
  const uint8_t payload[2] = {0x01, 0x00};
  return this->send_command_frame_(CMD_ENTER_CONFIG, payload, sizeof(payload)) && this->read_ack_frame_(CMD_ENTER_CONFIG, 500);
}

bool LD2451Component::exit_config_() {
  return this->send_command_frame_(CMD_EXIT_CONFIG, nullptr, 0) && this->read_ack_frame_(CMD_EXIT_CONFIG, 500);
}

bool LD2451Component::set_bluetooth_enabled_internal_(bool enabled) {
  const uint8_t payload[2] = {static_cast<uint8_t>(enabled ? 0x01 : 0x00), 0x00};
  if (!this->enter_config_()) {
    return false;
  }

  const bool ack = this->send_command_frame_(CMD_SET_BLUETOOTH, payload, sizeof(payload)) && this->read_ack_frame_(CMD_SET_BLUETOOTH, 500);
  const bool exited = this->exit_config_();
  return ack && exited;
}

bool LD2451Component::set_bluetooth_enabled(bool enabled) {
  const bool ok = this->set_bluetooth_enabled_internal_(enabled);
  if (ok) {
    this->bluetooth_enabled_state_ = enabled;
  }
  return ok;
}

void LD2451BluetoothSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    this->publish_state(!state);
    return;
  }

  const bool ok = this->parent_->set_bluetooth_enabled(state);
  this->publish_state(ok ? state : !state);
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
