#include <cassert>
#include <vector>

#include "../ack_stream.h"

using namespace esphome::ld2451;

namespace {

std::vector<uint8_t> build_cmd_frame(const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> frame = {
      0xFD, 0xFC, 0xFB, 0xFA,
      static_cast<uint8_t>(payload.size() & 0xFF),
      static_cast<uint8_t>((payload.size() >> 8) & 0xFF),
  };
  frame.insert(frame.end(), payload.begin(), payload.end());
  frame.insert(frame.end(), {0x04, 0x03, 0x02, 0x01});
  return frame;
}

std::vector<uint8_t> build_data_frame(const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> frame = {
      0xF4, 0xF3, 0xF2, 0xF1,
      static_cast<uint8_t>(payload.size() & 0xFF),
      static_cast<uint8_t>((payload.size() >> 8) & 0xFF),
  };
  frame.insert(frame.end(), payload.begin(), payload.end());
  frame.insert(frame.end(), {0xF8, 0xF7, 0xF6, 0xF5});
  return frame;
}

std::vector<uint8_t> build_ack_payload(uint16_t command, uint16_t status, const std::vector<uint8_t> &value = {}) {
  std::vector<uint8_t> payload = {
      static_cast<uint8_t>((command | 0x0100) & 0xFF),
      static_cast<uint8_t>(((command | 0x0100) >> 8) & 0xFF),
      static_cast<uint8_t>(status & 0xFF),
      static_cast<uint8_t>((status >> 8) & 0xFF),
  };
  payload.insert(payload.end(), value.begin(), value.end());
  return payload;
}

}  // namespace

int main() {
  {
    std::vector<uint8_t> rx = build_cmd_frame(build_ack_payload(0x0002, 0x0000, {0xAA}));
    std::vector<uint8_t> value;
    const AckScanResult result = scan_for_matching_ack(rx, 0x0002, 64, &value);
    assert(result == AckScanResult::MATCHED);
    assert(value.size() == 1);
    assert(value[0] == 0xAA);
    assert(rx.empty());
  }

  {
    std::vector<uint8_t> rx = build_data_frame({});
    const std::vector<uint8_t> ack = build_cmd_frame(build_ack_payload(0x0012, 0x0000));
    rx.insert(rx.end(), ack.begin(), ack.end());

    std::vector<uint8_t> value;
    const AckScanResult result = scan_for_matching_ack(rx, 0x0012, 64, &value);
    assert(result == AckScanResult::MATCHED);
    assert(value.empty());
    assert(rx.empty());
  }

  {
    std::vector<uint8_t> rx = build_cmd_frame(build_ack_payload(0x0003, 0x0000));
    const std::vector<uint8_t> expected = build_cmd_frame(build_ack_payload(0x0002, 0x0000, {0x55}));
    rx.insert(rx.end(), expected.begin(), expected.end());

    std::vector<uint8_t> value;
    const AckScanResult result = scan_for_matching_ack(rx, 0x0002, 64, &value);
    assert(result == AckScanResult::MATCHED);
    assert(value.size() == 1);
    assert(value[0] == 0x55);
    assert(rx.empty());
  }

  {
    // A partial ACK stays buffered for the next scan call; bytes are not blindly drained.
    std::vector<uint8_t> ack = build_cmd_frame(build_ack_payload(0x0002, 0x0000));
    std::vector<uint8_t> rx(ack.begin(), ack.begin() + 8);
    std::vector<uint8_t> value;
    AckScanResult result = scan_for_matching_ack(rx, 0x0002, 64, &value);
    assert(result == AckScanResult::NEED_MORE_DATA);
    assert(value.empty());
    rx.insert(rx.end(), ack.begin() + 8, ack.end());
    result = scan_for_matching_ack(rx, 0x0002, 64, &value);
    assert(result == AckScanResult::MATCHED);
    assert(value.empty());
    assert(rx.empty());
  }

  return 0;
}
