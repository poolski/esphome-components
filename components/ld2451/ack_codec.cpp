#include "ack_codec.h"

namespace esphome::ld2451 {

AckDecodeResult decode_ack(uint16_t command, const std::vector<uint8_t> &ack_payload, uint16_t max_ack_len) {
  AckDecodeResult result;

  if (ack_payload.size() < 4 || ack_payload.size() > max_ack_len) {
    return result;
  }

  const uint16_t ack_cmd = static_cast<uint16_t>(ack_payload[0]) | (static_cast<uint16_t>(ack_payload[1]) << 8);
  const uint16_t status = static_cast<uint16_t>(ack_payload[2]) | (static_cast<uint16_t>(ack_payload[3]) << 8);
  if (ack_cmd != static_cast<uint16_t>(command | 0x0100) || status != 0) {
    return result;
  }

  result.ok = true;
  result.value.assign(ack_payload.begin() + 4, ack_payload.end());
  return result;
}

}  // namespace esphome::ld2451
