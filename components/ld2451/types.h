#pragma once

#include <cstdint>

namespace esphome::ld2451 {

struct RuntimeConfig {
  uint8_t max_distance{100};
  uint8_t min_distance{0};
  uint8_t min_speed{0};
  uint8_t detection_direction{2};
  uint8_t no_target_delay{0};
  uint8_t trigger_count{1};
  uint8_t min_snr{0};
  float speed_correction{1.0f};
};

struct ParsedTarget {
  int angle{0};
  uint8_t distance{0};
  uint8_t direction{0};
  uint8_t speed{0};
  uint8_t snr{0};
};

}  // namespace esphome::ld2451
