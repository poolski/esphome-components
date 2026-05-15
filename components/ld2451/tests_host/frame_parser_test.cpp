#include <cassert>
#include <vector>

#include "../frame_parser.h"

using namespace esphome::ld2451;

int main() {
  FrameParser parser;
  std::vector<uint8_t> bytes = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x01, 0x8A,
      0x10, 0x01, 0x14, 0x22, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  parser.push(bytes.data(), bytes.size());

  ParsedFrame frame{};
  const bool ok = parser.pop(frame);
  assert(ok);
  assert(frame.target_count == 1);
  assert(frame.has_target);
  assert(frame.first_target.distance == 0x10);
  return 0;
}
