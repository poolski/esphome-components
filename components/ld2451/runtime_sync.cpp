#include "runtime_sync.h"

#include "config_state.h"

namespace esphome::ld2451 {

ReconcileResult reconcile_from_device(const RuntimeConfig &current, const RuntimeConfig &readback) {
  RuntimeConfig normalized = readback;
  normalize_distance_window(normalized);
  normalized.min_snr = coerce_native_min_snr(normalized.min_snr);

  ReconcileResult result;
  result.config = normalized;
  result.changed = !runtime_config_equal(current, normalized);
  return result;
}

bool should_run_sync(bool force_sync, bool sync_in_flight, uint32_t now_ms, uint32_t last_sync_ms, uint32_t interval_ms) {
  if (sync_in_flight) {
    return false;
  }
  if (force_sync) {
    return true;
  }
  return now_ms - last_sync_ms >= interval_ms;
}

}  // namespace esphome::ld2451
