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

  assert(should_run_sync(true, false, 1000, 0, 5000));
  assert(!should_run_sync(false, true, 6000, 0, 5000));
  assert(should_run_sync(false, false, 5000, 0, 5000));
  assert(!should_run_sync(false, false, 4999, 0, 5000));

  return 0;
}
