#include "frame_parser.h"

namespace esphome::ld2451 {

namespace {
static constexpr uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static constexpr uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static constexpr uint8_t CONFIG_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};

bool parse_target_block(const std::vector<uint8_t> &payload, size_t offset, ParsedTarget &target) {
  if (offset + 5 > payload.size()) {
    return false;
  }

  target.angle = static_cast<int>(payload[offset]) - 0x80;
  target.distance = payload[offset + 1];
  target.direction = payload[offset + 2];
  target.speed = payload[offset + 3];
  target.snr = payload[offset + 4];
  return true;
}

bool parse_payload(const std::vector<uint8_t> &payload, ParsedFrame &frame) {
  if (payload.size() < 2) {
    return false;
  }

  frame.target_count = payload[0];
  if (frame.target_count == 0) {
    return false;
  }

  const size_t required_size = 2 + static_cast<size_t>(frame.target_count) * 5;
  if (payload.size() < required_size) {
    return false;
  }

  const bool alarm = (payload[1] == 0x01);
  bool has_target = false;
  for (size_t i = 0; i < frame.target_count; i++) {
    ParsedTarget candidate{};
    if (!parse_target_block(payload, 2 + i * 5, candidate)) {
      return false;
    }
    if (!has_target || candidate.distance < frame.first_target.distance) {
      frame.first_target = candidate;
      has_target = true;
    }
  }

  frame.first_target.alarm = alarm;

  return has_target;
}
}  // namespace

void FrameParser::push(const uint8_t *data, size_t len) {
  if (data == nullptr || len == 0) {
    return;
  }
  this->buffer_.insert(this->buffer_.end(), data, data + len);
}

bool FrameParser::pop(ParsedFrame &frame) {
  if (this->buffer_.size() < 10) {
    return false;
  }

  size_t header_pos = this->buffer_.size();
  for (size_t i = 0; i + 4 <= this->buffer_.size(); i++) {
    if (this->buffer_[i] == DATA_HEADER[0] && this->buffer_[i + 1] == DATA_HEADER[1] &&
        this->buffer_[i + 2] == DATA_HEADER[2] && this->buffer_[i + 3] == DATA_HEADER[3]) {
      header_pos = i;
      break;
    }
  }

  if (header_pos == this->buffer_.size()) {
    // No data header found. Check if there's a config/ACK frame we can skip wholesale.
    for (size_t i = 0; i + 4 <= this->buffer_.size(); i++) {
      if (this->buffer_[i] == CONFIG_HEADER[0] && this->buffer_[i + 1] == CONFIG_HEADER[1] &&
          this->buffer_[i + 2] == CONFIG_HEADER[2] && this->buffer_[i + 3] == CONFIG_HEADER[3]) {
        // Need at least 6 bytes (header + length) to read the payload length.
        if (i + 6 > this->buffer_.size()) {
          break;
        }
        const uint16_t cfg_payload_len =
            static_cast<uint16_t>(this->buffer_[i + 4]) | (static_cast<uint16_t>(this->buffer_[i + 5]) << 8);
        const size_t cfg_frame_len = static_cast<size_t>(cfg_payload_len) + 10;
        if (i + cfg_frame_len > this->buffer_.size()) {
          // Incomplete config frame — wait for more data.
          break;
        }
        // Discard everything up to and including the complete config frame.
        this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + static_cast<long>(i + cfg_frame_len));
        return false;
      }
    }

    if (this->buffer_.size() > 3) {
      this->buffer_.erase(this->buffer_.begin(), this->buffer_.end() - 3);
    }
    return false;
  }

  if (header_pos > 0) {
    this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + static_cast<long>(header_pos));
  }

  if (this->buffer_.size() < 10) {
    return false;
  }

  const uint16_t payload_len = static_cast<uint16_t>(this->buffer_[4]) | (static_cast<uint16_t>(this->buffer_[5]) << 8);
  const size_t frame_len = static_cast<size_t>(payload_len) + 10;
  if (this->buffer_.size() < frame_len) {
    return false;
  }

  const size_t tail_pos = frame_len - 4;
  if (this->buffer_[tail_pos] != DATA_TAIL[0] || this->buffer_[tail_pos + 1] != DATA_TAIL[1] ||
      this->buffer_[tail_pos + 2] != DATA_TAIL[2] || this->buffer_[tail_pos + 3] != DATA_TAIL[3]) {
    this->buffer_.erase(this->buffer_.begin());
    return false;  // Fix 1: tail mismatch is not a successful pop
  }

  std::vector<uint8_t> payload;
  payload.reserve(payload_len);
  payload.insert(payload.end(), this->buffer_.begin() + 6, this->buffer_.begin() + 6 + payload_len);

  frame = ParsedFrame{};
  frame.has_target = parse_payload(payload, frame);

  this->buffer_.erase(this->buffer_.begin(), this->buffer_.begin() + static_cast<long>(frame_len));
  return true;
}

}  // namespace esphome::ld2451
