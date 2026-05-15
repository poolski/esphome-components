#include "target_publisher.h"

namespace esphome::ld2451 {

TargetOutput compute_target_output(const RuntimeConfig &cfg, const ParsedTarget &target) {
  if (target.distance < cfg.min_distance || target.distance > cfg.max_distance) {
    return {};
  }
  TargetOutput out;
  out.publish = true;
  out.corrected_speed = static_cast<float>(target.speed) * cfg.speed_correction;
  return out;
}

}  // namespace esphome::ld2451
