#pragma once

#include <cstdint>
#include <vector>

namespace esphome::ld2451 {

struct AckDecodeResult {
  bool ok{false};
  std::vector<uint8_t> value{};
};

AckDecodeResult decode_ack(uint16_t command, const std::vector<uint8_t> &ack_payload, uint16_t max_ack_len);

}  // namespace esphome::ld2451
