#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "types.h"

namespace esphome::ld2451 {

struct TargetOutput {
  bool publish{false};
  float corrected_speed{0.0f};
  // Mirrors ParsedTarget::alarm. True only when trigger_count consecutive detections were met.
  // Use this (not publish) to drive vehicle_detected.
  bool alarm{false};
};

struct IdleResetOutput {
  float angle{NAN};
  float distance{NAN};
  float speed{NAN};
  float snr{NAN};
  std::string direction{"None"};
};

TargetOutput compute_target_output(const RuntimeConfig &cfg, const ParsedTarget &target);
const char *direction_label(uint8_t direction_raw);
IdleResetOutput build_idle_reset_output();
bool should_publish_idle_reset(bool detection_active, bool idle_published, uint32_t now_ms, uint32_t last_detection_ms,
                               uint8_t no_target_delay_s);

}  // namespace esphome::ld2451
