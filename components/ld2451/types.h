#pragma once

#include <cstdint>

namespace esphome::ld2451 {

struct SensorSettings {
  // Range: 0x0A..0xFF (10..100 m). Device-stored; the device enforces this limit in hardware.
  // Not used as an ESPHome software filter — ESPHome publishes whatever the device reports.
  uint8_t max_distance{100};
  // Range: 0..100 m. Software-only filter: targets closer than this are not published to ESPHome.
  uint8_t min_distance{0};
  // Range: 0..120 km/h. Targets moving slower than this are not reported by the device. Device-stored.
  uint8_t min_speed{0};
  // 0 = away only (opposite-direction vehicles), 1 = approach only (same-direction vehicles), 2 = both. Device-stored.
  uint8_t detection_direction{2};
  // Range: 0..255 s. After the last detection the device continues reporting for this many seconds. Device-stored.
  uint8_t no_target_delay{0};
  // Range: 1..10. Consecutive detections required before the device reports a target (debounce). Device-stored.
  uint8_t trigger_count{1};
  // 0 = device default (equivalent to 4); 3..8 — higher value = lower sensitivity. Device-stored.
  uint8_t min_snr{0};
  // Software-only filter for speed publication. 0 disables the filter.
  uint8_t speed_publish_max_abs_angle{0};
  // Multiplier applied to the published speed value. Software-only.
  float speed_correction{1.0f};
};

struct ParsedTarget {
  int angle{0};
  uint8_t distance{0};
  // 0x00 = Approaching (towards sensor), 0x01 = Moving away.
  // jbeale monitor and Fiooodooor LD245X library agree on this encoding.
  // Madproforg's notes flag ambiguity in the official PDF; the
  // example code and third-party implementations use 0 = towards.
  uint8_t direction{0};
  uint8_t speed{0};
  uint8_t snr{0};
  // True when the device alarm flag (payload[1]) is set, meaning trigger_count consecutive
  // detections have been met. Gates vehicle_detected; sensor data is published regardless.
  bool alarm{false};
};

}  // namespace esphome::ld2451
