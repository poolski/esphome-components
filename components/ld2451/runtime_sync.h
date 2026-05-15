#pragma once

#include <cstdint>

#include "types.h"

namespace esphome::ld2451 {

struct ReconcileResult {
  RuntimeConfig config{};
  bool changed{false};
};

ReconcileResult reconcile_from_device(const RuntimeConfig &current, RuntimeConfig readback);
bool should_run_sync(bool force_sync, bool sync_in_flight, uint32_t now_ms, uint32_t last_sync_ms, uint32_t interval_ms);

}  // namespace esphome::ld2451
