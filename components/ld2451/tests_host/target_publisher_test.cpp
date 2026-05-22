#include <cassert>
#include <vector>

#include "../target_publisher.h"

using namespace esphome::ld2451;

int main() {
  SensorSettings cfg{};
  cfg.min_distance = 5;
  cfg.max_distance = 10;  // device-side only; does not affect ESPHome publish filter
  cfg.speed_correction = 1.1f;

  ParsedTarget target{};
  target.distance = 7;
  target.speed = 20;

  target.alarm = true;
  const TargetOutput out = compute_target_output(cfg, target);
  assert(out.publish);
  assert(out.alarm == true);
  assert(out.corrected_speed > 21.9f && out.corrected_speed < 22.1f);

  // alarm=false propagates even when target is in range
  target.alarm = false;
  const TargetOutput no_alarm_out = compute_target_output(cfg, target);
  assert(no_alarm_out.publish);
  assert(no_alarm_out.alarm == false);

  // targets beyond max_distance still publish (device-side filter only)
  target.distance = 34;
  assert(compute_target_output(cfg, target).publish);

  // only min_distance is a software filter: targets below it are suppressed
  target.distance = 4;
  assert(!compute_target_output(cfg, target).publish);

  std::vector<ParsedTarget> targets{};
  ParsedTarget near_target{};
  near_target.distance = 4;
  targets.push_back(near_target);
  ParsedTarget far_target{};
  far_target.distance = 7;
  targets.push_back(far_target);
  ParsedTarget selected{};
  assert(select_nearest_qualifying_target(cfg, targets, selected));
  assert(selected.distance == 7);

  targets.clear();
  targets.push_back(near_target);
  assert(!select_nearest_qualifying_target(cfg, targets, selected));

  assert(direction_label(0x00) == std::string("Approaching"));
  assert(direction_label(0x01) == std::string("Moving away"));

  assert(should_publish_idle_reset(true, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, false, 3999, 1000, 3));
  assert(!should_publish_idle_reset(false, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, true, 4000, 1000, 3));
  return 0;
}
