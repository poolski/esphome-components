#pragma once

#include <cstdint>
#include <string>

#include "types.h"

namespace esphome::ld2451 {

struct TargetOutput {
  bool publish{false};
  float corrected_speed{0.0f};
};

struct IdleResetOutput {
  int angle{0};
  uint8_t distance{0};
  float speed{0.0f};
  uint8_t snr{0};
  std::string direction{"None"};
};

TargetOutput compute_target_output(const RuntimeConfig &cfg, const ParsedTarget &target);
const char *direction_label(uint8_t direction_raw);
IdleResetOutput build_idle_reset_output();
bool should_publish_idle_reset(bool detection_active, bool idle_published, uint32_t now_ms, uint32_t last_detection_ms,
                               uint8_t no_target_delay_s);

}  // namespace esphome::ld2451
