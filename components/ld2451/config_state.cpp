#include "config_state.h"

namespace esphome::ld2451 {

void normalize_distance_window(SensorSettings &cfg) {
  if (cfg.min_distance > cfg.max_distance) {
    cfg.min_distance = cfg.max_distance;
  }
}

uint8_t coerce_native_min_snr(uint8_t value) {
  if (value == 0 || (value >= 3 && value <= 8)) {
    return value;
  }
  return 0;
}

uint8_t map_app_snr_to_native(uint8_t app_value) {
  if (app_value == 0) {
    return 0;
  }
  if (app_value <= 8) {
    return 3;
  }
  if (app_value <= 24) {
    return 4;
  }
  if (app_value <= 32) {
    return 5;
  }
  if (app_value <= 40) {
    return 6;
  }
  if (app_value <= 52) {
    return 7;
  }
  return 8;
}

bool settings_equal(const SensorSettings &a, const SensorSettings &b) {
  return a.max_distance == b.max_distance && a.min_distance == b.min_distance && a.min_speed == b.min_speed &&
         a.detection_direction == b.detection_direction && a.no_target_delay == b.no_target_delay &&
         a.trigger_count == b.trigger_count && a.min_snr == b.min_snr && a.speed_correction == b.speed_correction;
}

bool settings_readback_fields_equal(const SensorSettings &a, const SensorSettings &b) {
  return a.max_distance == b.max_distance && a.detection_direction == b.detection_direction &&
         a.min_speed == b.min_speed && a.no_target_delay == b.no_target_delay && a.trigger_count == b.trigger_count &&
         a.min_snr == b.min_snr;
}

}  // namespace esphome::ld2451
