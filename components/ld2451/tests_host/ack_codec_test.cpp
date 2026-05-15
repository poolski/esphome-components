#include <cassert>
#include <string>
#include <vector>

#include "../ack_codec.h"

using namespace esphome::ld2451;

int main() {
  const std::vector<uint8_t> ack_payload = {0x02, 0x01, 0x00, 0x00, 0xAA};
  AckDecodeResult result = decode_ack(0x0002, ack_payload, 64);
  assert(result.ok);
  assert(result.value.size() == 1);
  assert(result.value[0] == 0xAA);

  const std::vector<uint8_t> too_short = {0x02, 0x01, 0x00};
  assert(!decode_ack(0x0002, too_short, 64).ok);

  FirmwareVersionInfo fw{};
  const std::vector<uint8_t> fw_payload = {0x51, 0x24, 0x01, 0x01, 0x10, 0x15, 0x05, 0x24};
  assert(decode_firmware_version(fw_payload, fw));
  assert(fw.fw_type == 0x2451);
  assert(fw.major == 0x0101);
  assert(fw.minor == 0x24051510);

  const std::vector<uint8_t> fw_short = {0x51, 0x24, 0x01};
  assert(!decode_firmware_version(fw_short, fw));

  assert(format_firmware_version(fw) == "V1.01.24051510");

  return 0;
}
