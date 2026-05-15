#include <cassert>

#include "../target_publisher.h"

using namespace esphome::ld2451;

int main() {
  RuntimeConfig cfg{};
  cfg.min_distance = 5;
  cfg.max_distance = 10;
  cfg.speed_correction = 1.1f;

  ParsedTarget target{};
  target.distance = 7;
  target.speed = 20;

  const TargetOutput out = compute_target_output(cfg, target);
  assert(out.publish);
  assert(out.corrected_speed > 21.9f && out.corrected_speed < 22.1f);

  target.distance = 20;
  assert(!compute_target_output(cfg, target).publish);
  return 0;
}
