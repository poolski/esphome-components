#include "frame_parser.h"

namespace esphome::ld2451 {

namespace {
static constexpr uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static constexpr uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};

bool parse_payload(const std::vector<uint8_t> &payload, ParsedFrame &frame) {
  if (payload.size() < 2) {
    return false;
  }

  frame.target_count = payload[0];
  frame.alarm = payload[1] != 0;
  if (frame.target_count == 0 || payload.size() < 7) {
    return false;
  }

  frame.first_target.angle = static_cast<int>(payload[2]) - 0x80;
  frame.first_target.distance = payload[3];
  frame.first_target.direction = payload[4];
  frame.first_target.speed = payload[5];
  frame.first_target.snr = payload[6];
  return true;
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
    return true;
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
