#include <cassert>

#include "../runtime/config_state.h"

using namespace esphome::ld2451;

int main() {
  RuntimeConfig cfg{};
  cfg.max_distance = 10;
  cfg.min_distance = 20;
  normalize_distance_window(cfg);
  assert(cfg.min_distance == 10);

  assert(map_app_snr_to_native(0) == 0);
  assert(map_app_snr_to_native(1) == 3);
  assert(map_app_snr_to_native(64) == 8);
  assert(coerce_native_min_snr(2) == 0);
  assert(coerce_native_min_snr(7) == 7);
  return 0;
}
