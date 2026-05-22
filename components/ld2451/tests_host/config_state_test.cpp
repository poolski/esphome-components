#include <cassert>

#include "../config_state.h"

using namespace esphome::ld2451;

int main() {
  SensorSettings cfg{};
  cfg.max_distance = 10;
  cfg.min_distance = 20;
  normalize_distance_window(cfg);
  assert(cfg.min_distance == 10);

  assert(map_app_snr_to_native(0) == 0);
  assert(map_app_snr_to_native(1) == 3);
  assert(map_app_snr_to_native(64) == 8);
  assert(coerce_native_min_snr(2) == 0);
  assert(coerce_native_min_snr(7) == 7);

  SensorSettings baseline{};
  SensorSettings same{};
  assert(settings_equal(baseline, same));

  SensorSettings changed = baseline;
  changed.min_distance = 5;
  assert(!settings_equal(baseline, changed));

  SensorSettings readback_a{};
  readback_a.max_distance = 20;
  readback_a.detection_direction = 1;
  readback_a.min_speed = 4;
  readback_a.no_target_delay = 3;
  readback_a.trigger_count = 2;
  readback_a.min_snr = 7;
  readback_a.min_distance = 1;
  readback_a.speed_correction = 0.8f;

  SensorSettings readback_b = readback_a;
  readback_b.min_distance = 99;
  readback_b.speed_correction = 1.5f;
  assert(settings_readback_fields_equal(readback_a, readback_b));

  readback_b.min_snr = 6;
  assert(!settings_readback_fields_equal(readback_a, readback_b));
  return 0;
}
