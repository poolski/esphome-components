#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../internal/types.h"

namespace esphome::ld2451 {

struct ParsedFrame {
  uint8_t target_count{0};
  bool alarm{false};
  bool has_target{false};
  ParsedTarget first_target{};
};

class FrameParser {
 public:
  void push(const uint8_t *data, size_t len);
  bool pop(ParsedFrame &frame);

 private:
  std::vector<uint8_t> buffer_;
};

}  // namespace esphome::ld2451
