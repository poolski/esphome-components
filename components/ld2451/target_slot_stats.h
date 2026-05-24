#pragma once

#include <cstdint>

#include "target_publisher.h"

namespace esphome::ld2451 {

class RollingStat {
 public:
  void reset();
  void add(float value);

  bool has_samples() const { return this->count_ > 0; }
  uint32_t count() const { return this->count_; }
  float min() const { return this->min_; }
  float max() const { return this->max_; }
  float average() const;

 private:
  uint32_t count_{0};
  float min_{0.0f};
  float max_{0.0f};
  float sum_{0.0f};
};

class LiveTargetSlotStats {
 public:
  void reset();
  void ingest(const LiveTargetOutput &output);

  RollingStat distance;
  RollingStat speed;
  RollingStat speed_mph;
  RollingStat snr;
};

}  // namespace esphome::ld2451
