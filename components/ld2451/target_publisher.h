#pragma once

#include "types.h"

namespace esphome::ld2451 {

struct TargetOutput {
  bool publish{false};
  float corrected_speed{0.0f};
};

TargetOutput compute_target_output(const RuntimeConfig &cfg, const ParsedTarget &target);

}  // namespace esphome::ld2451
