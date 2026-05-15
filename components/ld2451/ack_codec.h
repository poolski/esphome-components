#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esphome::ld2451 {

struct AckDecodeResult {
  bool ok{false};
  std::vector<uint8_t> value{};
};

struct FirmwareVersionInfo {
  uint16_t fw_type{0};
  uint16_t major{0};
  uint32_t minor{0};
};

AckDecodeResult decode_ack(uint16_t command, const std::vector<uint8_t> &ack_payload, uint16_t max_ack_len);
bool decode_firmware_version(const std::vector<uint8_t> &ret_payload, FirmwareVersionInfo &out);
std::string format_firmware_version(const FirmwareVersionInfo &fw);

}  // namespace esphome::ld2451
