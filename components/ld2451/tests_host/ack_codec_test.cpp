#include <cassert>
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

  return 0;
}
