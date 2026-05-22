#include "target_publisher.h"

#include <cmath>

namespace esphome::ld2451 {

static constexpr float kPi = 3.14159265358979323846f;

TargetOutput compute_target_output(const SensorSettings &cfg, const ParsedTarget &target) {
  // min_distance is a software-only filter; max_distance is enforced by the device.
  if (target.distance < cfg.min_distance) {
    return {};
  }
  TargetOutput out;
  out.publish = true;
  out.alarm = target.alarm;
  out.corrected_speed = static_cast<float>(target.speed) * cfg.speed_correction;
  out.corrected_speed_mph = out.corrected_speed * 0.6213712f;
  return out;
}

std::array<LiveTargetOutput, kLiveTargetSlotCount> build_live_target_outputs(
    const SensorSettings &cfg, const std::vector<ParsedTarget> &targets) {
  std::array<LiveTargetOutput, kLiveTargetSlotCount> out{};
  for (size_t i = 0; i < out.size() && i < targets.size(); i++) {
    out[i].present = true;
    out[i].target = targets[i];
    const float angle_rad = static_cast<float>(targets[i].angle) * (kPi / 180.0f);
    out[i].x = roundf(static_cast<float>(targets[i].distance) * cosf(angle_rad));
    out[i].y = roundf(static_cast<float>(targets[i].distance) * sinf(angle_rad));
    out[i].corrected_speed = static_cast<float>(targets[i].speed) * cfg.speed_correction;
    out[i].corrected_speed_mph = out[i].corrected_speed * 0.6213712f;
  }
  return out;
}

bool select_nearest_qualifying_target(const SensorSettings &cfg, const std::vector<ParsedTarget> &targets,
                                      ParsedTarget &selected) {
  bool has_selected = false;
  for (const auto &target : targets) {
    if (target.distance < cfg.min_distance) {
      continue;
    }
    if (!has_selected || target.distance < selected.distance) {
      selected = target;
      has_selected = true;
    }
  }
  return has_selected;
}

const char *direction_label(uint8_t direction_raw) { return direction_raw == 0x00 ? "Approaching" : "Moving away"; }

bool should_publish_idle_reset(bool detection_active, bool idle_published, uint32_t now_ms, uint32_t last_detection_ms,
                               uint8_t no_target_delay_s) {
  if (!detection_active || idle_published) {
    return false;
  }
  const uint32_t delay_ms = static_cast<uint32_t>(no_target_delay_s) * 1000U;
  return now_ms - last_detection_ms >= delay_ms;
}

}  // namespace esphome::ld2451
