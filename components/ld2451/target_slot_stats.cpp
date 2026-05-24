#include "target_slot_stats.h"

#include <algorithm>

namespace esphome::ld2451 {

void RollingStat::reset() {
  this->count_ = 0;
  this->min_ = 0.0f;
  this->max_ = 0.0f;
  this->sum_ = 0.0f;
}

void RollingStat::add(float value) {
  if (this->count_ == 0) {
    this->min_ = value;
    this->max_ = value;
    this->sum_ = value;
    this->count_ = 1;
    return;
  }

  this->min_ = std::min(this->min_, value);
  this->max_ = std::max(this->max_, value);
  this->sum_ += value;
  this->count_++;
}

float RollingStat::average() const {
  if (this->count_ == 0) {
    return 0.0f;
  }
  return this->sum_ / static_cast<float>(this->count_);
}

void LiveTargetSlotStats::reset() {
  this->distance.reset();
  this->speed.reset();
  this->speed_mph.reset();
  this->snr.reset();
}

void LiveTargetSlotStats::ingest(const LiveTargetOutput &output) {
  if (!output.present) {
    this->reset();
    return;
  }

  this->distance.add(static_cast<float>(output.target.distance));
  this->speed.add(output.corrected_speed);
  this->speed_mph.add(output.corrected_speed_mph);
  this->snr.add(static_cast<float>(output.target.snr));
}

}  // namespace esphome::ld2451
