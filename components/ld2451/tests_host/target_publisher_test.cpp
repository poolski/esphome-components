#include <cassert>
#include <cmath>

#include "../target_publisher.h"

using namespace esphome::ld2451;

int main() {
  RuntimeConfig cfg{};
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

  assert(direction_label(0x01) == std::string("Approaching"));
  assert(direction_label(0x00) == std::string("Moving away"));

  const IdleResetOutput reset = build_idle_reset_output();
  assert(std::isnan(reset.angle));
  assert(std::isnan(reset.distance));
  assert(std::isnan(reset.speed));
  assert(std::isnan(reset.snr));
  assert(reset.direction == std::string("None"));

  assert(should_publish_idle_reset(true, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, false, 3999, 1000, 3));
  assert(!should_publish_idle_reset(false, false, 4000, 1000, 3));
  assert(!should_publish_idle_reset(true, true, 4000, 1000, 3));
  return 0;
}
