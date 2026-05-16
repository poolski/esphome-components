#pragma once

#include <cstdint>
#include <vector>

namespace esphome::ld2451 {

enum class AckScanResult {
  NEED_MORE_DATA,
  MATCHED,
};

AckScanResult scan_for_matching_ack(std::vector<uint8_t> &rx_bytes, uint16_t command, uint16_t max_ack_payload_len,
                                    std::vector<uint8_t> *ret);

}  // namespace esphome::ld2451
