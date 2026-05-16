#pragma once

#include "types.h"

namespace esphome::ld2451 {

void normalize_distance_window(RuntimeConfig &cfg);
uint8_t coerce_native_min_snr(uint8_t value);
uint8_t map_app_snr_to_native(uint8_t app_value);
bool runtime_config_equal(const RuntimeConfig &a, const RuntimeConfig &b);
bool runtime_config_readback_fields_equal(const RuntimeConfig &a, const RuntimeConfig &b);

}  // namespace esphome::ld2451
