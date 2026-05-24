#include <cassert>

#include "../target_slot_stats.h"

using namespace esphome::ld2451;

int main() {
  RollingStat stats{};
  assert(!stats.has_samples());

  stats.add(10.0f);
  stats.add(14.0f);
  stats.add(12.0f);
  assert(stats.has_samples());
  assert(stats.min() == 10.0f);
  assert(stats.max() == 14.0f);
  assert(stats.count() == 3);
  assert(stats.average() > 11.9f && stats.average() < 12.1f);

  LiveTargetSlotStats slot{};
  LiveTargetOutput first{};
  first.present = true;
  first.target.distance = 4;
  first.corrected_speed = 8.0f;
  first.corrected_speed_mph = 4.9709696f;
  first.target.snr = 20;
  slot.ingest(first);

  LiveTargetOutput second = first;
  second.target.distance = 8;
  second.corrected_speed = 10.0f;
  second.corrected_speed_mph = 6.213712f;
  second.target.snr = 24;
  slot.ingest(second);

  assert(slot.distance.has_samples());
  assert(slot.distance.min() == 4.0f);
  assert(slot.distance.max() == 8.0f);
  assert(slot.distance.count() == 2);
  assert(slot.distance.average() > 5.9f && slot.distance.average() < 6.1f);

  assert(slot.speed.min() == 8.0f);
  assert(slot.speed.max() == 10.0f);
  assert(slot.speed.count() == 2);

  slot.ingest(LiveTargetOutput{});
  assert(!slot.distance.has_samples());
  assert(!slot.speed.has_samples());
  assert(!slot.speed_mph.has_samples());
  assert(!slot.snr.has_samples());
  return 0;
}
