#include "ack_codec.h"

#include <cstdio>

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

bool decode_firmware_version(const std::vector<uint8_t> &ret_payload, FirmwareVersionInfo &out) {
  if (ret_payload.size() < 8) {
    return false;
  }

  out.fw_type = static_cast<uint16_t>(ret_payload[0]) | (static_cast<uint16_t>(ret_payload[1]) << 8);
  out.major = static_cast<uint16_t>(ret_payload[2]) | (static_cast<uint16_t>(ret_payload[3]) << 8);
  out.minor = static_cast<uint32_t>(ret_payload[4]) | (static_cast<uint32_t>(ret_payload[5]) << 8) |
              (static_cast<uint32_t>(ret_payload[6]) << 16) | (static_cast<uint32_t>(ret_payload[7]) << 24);
  return true;
}

std::string format_firmware_version(const FirmwareVersionInfo &fw) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "V%u.%02u.%08X", static_cast<unsigned int>((fw.major >> 8) & 0xFF),
                static_cast<unsigned int>(fw.major & 0xFF), static_cast<unsigned int>(fw.minor));
  return std::string(buffer);
}

}  // namespace esphome::ld2451
