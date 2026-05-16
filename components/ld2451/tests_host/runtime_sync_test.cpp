#include <cassert>
#include <type_traits>

#include "../runtime_sync.h"

using namespace esphome::ld2451;

static_assert(std::is_same_v<decltype(&reconcile_from_device), ReconcileResult (*)(const RuntimeConfig &, RuntimeConfig)>);

int main() {
  RuntimeConfig current{};
  RuntimeConfig readback{};
  readback.max_distance = 80;

  const ReconcileResult r = reconcile_from_device(current, readback);
  assert(r.changed);
  assert(r.config.max_distance == 80);

  const ReconcileResult same = reconcile_from_device(r.config, r.config);
  assert(!same.changed);

  RuntimeConfig distance_current{};
  distance_current.min_distance = 10;
  distance_current.max_distance = 80;
  RuntimeConfig distance_readback = distance_current;
  distance_readback.min_distance = 90;
  distance_readback.max_distance = 80;

  const ReconcileResult normalized_distance = reconcile_from_device(distance_current, distance_readback);
  assert(normalized_distance.changed);
  assert(normalized_distance.config.max_distance == 80);
  assert(normalized_distance.config.min_distance == 80);

  RuntimeConfig snr_current{};
  snr_current.min_snr = 2;
  RuntimeConfig snr_readback = snr_current;

  const ReconcileResult coerced_snr = reconcile_from_device(snr_current, snr_readback);
  assert(coerced_snr.changed);
  assert(coerced_snr.config.min_snr == 0);

  assert(should_run_sync(true, false, 1, 1, 5000));
  assert(should_run_sync(true, false, 1000, 0, 5000));
  assert(!should_run_sync(true, true, 1000, 0, 5000));
  assert(!should_run_sync(false, true, 6000, 0, 5000));
  assert(should_run_sync(false, false, 5000, 0, 5000));
  assert(!should_run_sync(false, false, 4999, 0, 5000));

  return 0;
}
