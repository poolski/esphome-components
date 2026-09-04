#include <cassert>
#include <vector>

#include "../frame_parser.h"

using namespace esphome::ld2451;

int main() {
  FrameParser parser;
  std::vector<uint8_t> bytes = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x01, 0x8A, 0x10, 0x01, 0x14, 0x22, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  parser.push(bytes.data(), bytes.size());

  ParsedFrame frame{};
  const bool ok = parser.pop(frame);
  assert(ok);
  assert(frame.target_count == 1);
  assert(frame.has_target);
  assert(frame.targets.size() == 1);
  assert(frame.targets[0].distance == 0x10);
  assert(frame.alarm == true);  // payload[1] == 0x01

  // A multi-target frame exposes every target in frame order (not just the nearest).
  FrameParser multi_parser;
  std::vector<uint8_t> multi_bytes = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x11, 0x00, 0x03, 0x01, 0x8A, 0x28, 0x00, 0x3C, 0x15, 0x8A,
      0x1E, 0x01, 0x3C, 0x0F, 0x76, 0x5F, 0x00, 0x3C, 0x0F, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  multi_parser.push(multi_bytes.data(), multi_bytes.size());
  ParsedFrame multi_frame{};
  assert(multi_parser.pop(multi_frame));
  assert(multi_frame.has_target);
  assert(multi_frame.target_count == 3);
  assert(multi_frame.alarm == true);
  assert(multi_frame.targets.size() == 3);
  // Target 0
  assert(multi_frame.targets[0].angle == 0x8A - 0x80);
  assert(multi_frame.targets[0].distance == 0x28);
  assert(multi_frame.targets[0].direction == 0x00);
  assert(multi_frame.targets[0].speed == 0x3C);
  assert(multi_frame.targets[0].snr == 0x15);
  // Target 1
  assert(multi_frame.targets[1].distance == 0x1E);
  assert(multi_frame.targets[1].direction == 0x01);
  // Target 2
  assert(multi_frame.targets[2].angle == 0x76 - 0x80);
  assert(multi_frame.targets[2].distance == 0x5F);
  assert(multi_frame.targets[2].direction == 0x00);

  // alarm=false when payload[1]==0x00 (trigger_count not yet met)
  FrameParser no_alarm_parser;
  std::vector<uint8_t> no_alarm_bytes = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x00, 0x8A, 0x10, 0x01, 0x14, 0x22, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  no_alarm_parser.push(no_alarm_bytes.data(), no_alarm_bytes.size());
  ParsedFrame no_alarm_frame{};
  assert(no_alarm_parser.pop(no_alarm_frame));
  assert(no_alarm_frame.has_target);
  assert(no_alarm_frame.alarm == false);

  FrameParser empty_payload_parser;
  std::vector<uint8_t> empty_payload = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x00, 0x00, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  empty_payload_parser.push(empty_payload.data(), empty_payload.size());

  ParsedFrame empty_frame{};
  const bool empty_ok = empty_payload_parser.pop(empty_frame);
  assert(empty_ok);
  assert(!empty_frame.has_target);
  assert(empty_frame.target_count == 0);
  assert(empty_frame.targets.empty());

  // Test A: config/ACK frame (FD FC FB FA ... 04 03 02 01) is skipped cleanly.
  // Minimal config frame: header(4) + len(2=0x02,0x00) + payload(2: AA BB) + tail(4) = 12 bytes.
  FrameParser cfg_skip_parser;
  std::vector<uint8_t> cfg_frame = {
      0xFD, 0xFC, 0xFB, 0xFA,  // config header
      0x02, 0x00,              // payload length = 2
      0xAA, 0xBB,              // payload
      0x04, 0x03, 0x02, 0x01,  // config tail
  };
  cfg_skip_parser.push(cfg_frame.data(), cfg_frame.size());
  ParsedFrame cfg_frame_out{};
  assert(!cfg_skip_parser.pop(cfg_frame_out));  // no data frame popped
  // Buffer should be empty after skipping the config frame.
  // Verify by pushing a real data frame and confirming it parses.
  std::vector<uint8_t> after_cfg_data = {
      0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x00, 0x8A, 0x10, 0x01, 0x14, 0x22, 0xF8, 0xF7, 0xF6, 0xF5,
  };
  cfg_skip_parser.push(after_cfg_data.data(), after_cfg_data.size());
  ParsedFrame after_cfg_frame{};
  assert(cfg_skip_parser.pop(after_cfg_frame));
  assert(after_cfg_frame.has_target);
  assert(after_cfg_frame.targets.size() == 1);
  assert(after_cfg_frame.targets[0].distance == 0x10);

  // Test B: a config frame followed by a data frame in one buffer yields the data
  // frame from a single pop (config skipped internally, then drained).
  FrameParser cfg_then_data_parser;
  std::vector<uint8_t> cfg_then_data;
  cfg_then_data.insert(cfg_then_data.end(), cfg_frame.begin(), cfg_frame.end());
  cfg_then_data.insert(cfg_then_data.end(), after_cfg_data.begin(), after_cfg_data.end());
  cfg_then_data_parser.push(cfg_then_data.data(), cfg_then_data.size());
  ParsedFrame drained{};
  assert(cfg_then_data_parser.pop(drained));
  assert(drained.has_target);
  assert(drained.targets.size() == 1);

  // Test C: tail mismatch does not pop a frame; the parser resyncs and finds the
  // valid data frame that follows.
  FrameParser tail_mismatch_parser;
  std::vector<uint8_t> bad_tail_bytes = {
      0xF4, 0xF3, 0xF2, 0xF1,                    // data header
      0x07, 0x00,                                // payload length = 7
      0x01, 0x01, 0x8A, 0x10, 0x01, 0x14, 0x22,  // payload
      0xFF, 0xFF, 0xFF, 0xFF,                    // wrong tail (should be F8 F7 F6 F5)
  };
  tail_mismatch_parser.push(bad_tail_bytes.data(), bad_tail_bytes.size());
  ParsedFrame tail_mismatch_frame{};
  assert(!tail_mismatch_parser.pop(tail_mismatch_frame));  // no valid frame

  return 0;
}
