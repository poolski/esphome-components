#include "target_publisher.h"

namespace esphome::ld2451 {

TargetOutput compute_target_output(const RuntimeConfig &cfg, const ParsedTarget &target) {
  // min_distance is a software-only filter; max_distance is enforced by the device.
  if (target.distance < cfg.min_distance) {
    return {};
  }
  TargetOutput out;
  out.publish = true;
  out.alarm = target.alarm;
  out.corrected_speed = static_cast<float>(target.speed) * cfg.speed_correction;
  return out;
}

const char *direction_label(uint8_t direction_raw) { return direction_raw == 0x01 ? "Approaching" : "Moving away"; }

IdleResetOutput build_idle_reset_output() { return {}; }

bool should_publish_idle_reset(bool detection_active, bool idle_published, uint32_t now_ms, uint32_t last_detection_ms,
                               uint8_t no_target_delay_s) {
  if (!detection_active || idle_published) {
    return false;
  }
  const uint32_t delay_ms = static_cast<uint32_t>(no_target_delay_s) * 1000U;
  return now_ms - last_detection_ms >= delay_ms;
}

}  // namespace esphome::ld2451
