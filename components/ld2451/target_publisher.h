#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "types.h"

namespace esphome::ld2451 {

constexpr size_t kLiveTargetSlotCount = 3;

struct TargetOutput {
  bool publish{false};
  float corrected_speed{0.0f};
  float corrected_speed_mph{0.0f};
  // Mirrors ParsedTarget::alarm. True only when trigger_count consecutive detections were met.
  // Use this (not publish) to drive vehicle_detected.
  bool alarm{false};
};

struct LiveTargetOutput {
  bool present{false};
  ParsedTarget target{};
  float corrected_speed{0.0f};
  float corrected_speed_mph{0.0f};
};

TargetOutput compute_target_output(const SensorSettings &cfg, const ParsedTarget &target);
std::array<LiveTargetOutput, kLiveTargetSlotCount> build_live_target_outputs(
    const SensorSettings &cfg, const std::vector<ParsedTarget> &targets);
bool select_nearest_qualifying_target(const SensorSettings &cfg, const std::vector<ParsedTarget> &targets,
                                      ParsedTarget &selected);
const char *direction_label(uint8_t direction_raw);
bool should_publish_idle_reset(bool detection_active, bool idle_published, uint32_t now_ms, uint32_t last_detection_ms,
                               uint8_t no_target_delay_s);

}  // namespace esphome::ld2451
