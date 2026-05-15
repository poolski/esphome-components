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

  assert(direction_label(0x01) == std::string("Approaching"));
  assert(direction_label(0x00) == std::string("Moving away"));

  const IdleResetOutput reset = build_idle_reset_output();
  assert(reset.angle == 0);
  assert(reset.distance == 0);
  assert(reset.speed == 0.0f);
  assert(reset.snr == 0);
  assert(reset.direction == std::string("None"));

  assert(should_publish_idle_reset(true, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, false, 3999, 1000, 3));
  assert(!should_publish_idle_reset(false, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, true, 4000, 1000, 3));
  return 0;
}
