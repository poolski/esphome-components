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
  assert(frame.first_target.alarm == true);  // payload[1] == 0x01

  // alarm=false when payload[1]==0x00 (trigger_count not yet met)
  FrameParser no_alarm_parser;
  std::vector<uint8_t> no_alarm_bytes = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x00, 0x8A,
      0x10, 0x01, 0x14, 0x22, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  no_alarm_parser.push(no_alarm_bytes.data(), no_alarm_bytes.size());
  ParsedFrame no_alarm_frame{};
  assert(no_alarm_parser.pop(no_alarm_frame));
  assert(no_alarm_frame.has_target);
  assert(no_alarm_frame.first_target.alarm == false);

  FrameParser empty_payload_parser;
  std::vector<uint8_t> empty_payload = {
      0xF4, 0xF3, 0xF2, 0xF1,
      0x00, 0x00,
      0xF8, 0xF7, 0xF6, 0xF5,
  };
  empty_payload_parser.push(empty_payload.data(), empty_payload.size());

  ParsedFrame empty_frame{};
  const bool empty_ok = empty_payload_parser.pop(empty_frame);
  assert(empty_ok);
  assert(!empty_frame.has_target);
  assert(empty_frame.target_count == 0);

  return 0;
}
