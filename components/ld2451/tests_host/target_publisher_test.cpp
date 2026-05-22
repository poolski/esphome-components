#include <cassert>
#include <cmath>
#include <vector>

#include "../target_publisher.h"

using namespace esphome::ld2451;

int main() {
  SensorSettings cfg{};
  cfg.min_distance = 5;
  cfg.max_distance = 10;  // device-side only; does not affect ESPHome publish filter
  cfg.speed_correction = 1.1f;
  cfg.speed_publish_min_snr = 18;
  cfg.speed_publish_max_abs_angle = 65;

  ParsedTarget target{};
  target.distance = 7;
  target.speed = 20;
  target.snr = 22;
  target.angle = 12;

  target.alarm = true;
  const TargetOutput out = compute_target_output(cfg, target);
  assert(out.publish);
  assert(out.alarm == true);
  assert(out.speed_publish);
  assert(out.corrected_speed > 21.9f && out.corrected_speed < 22.1f);
  assert(out.corrected_speed_mph > 13.6f && out.corrected_speed_mph < 13.7f);

  // alarm=false propagates even when target is in range
  target.alarm = false;
  const TargetOutput no_alarm_out = compute_target_output(cfg, target);
  assert(no_alarm_out.publish);
  assert(no_alarm_out.alarm == false);

  target.snr = 12;
  target.angle = 70;
  const TargetOutput filtered_speed = compute_target_output(cfg, target);
  assert(filtered_speed.publish);
  assert(!filtered_speed.speed_publish);
  assert(std::isnan(filtered_speed.corrected_speed));
  assert(std::isnan(filtered_speed.corrected_speed_mph));

  std::vector<ParsedTarget> live_targets{};
  ParsedTarget live0{};
  live0.distance = 2;
  live0.angle = 0;
  live0.speed = 10;
  live0.snr = 30;
  live_targets.push_back(live0);
  ParsedTarget live1{};
  live1.distance = 7;
  live1.angle = 90;
  live1.speed = 20;
  live1.snr = 30;
  live_targets.push_back(live1);
  ParsedTarget live2{};
  live2.distance = 9;
  live2.angle = -90;
  live2.speed = 30;
  live2.snr = 10;
  live_targets.push_back(live2);
  ParsedTarget live3{};
  live3.distance = 11;
  live3.speed = 40;
  live_targets.push_back(live3);

  const auto live_outputs = build_live_target_outputs(cfg, live_targets);
  assert(live_outputs.size() == kLiveTargetSlotCount);
  assert(live_outputs[0].present);
  assert(live_outputs[0].target.distance == 2);
  assert(live_outputs[0].speed_publish);
  assert(live_outputs[0].x > 1.9f && live_outputs[0].x < 2.1f);
  assert(live_outputs[0].y > -0.1f && live_outputs[0].y < 0.1f);
  assert(live_outputs[0].corrected_speed > 10.9f && live_outputs[0].corrected_speed < 11.1f);
  assert(live_outputs[0].corrected_speed_mph > 6.7f && live_outputs[0].corrected_speed_mph < 6.9f);
  assert(live_outputs[1].present);
  assert(live_outputs[1].target.distance == 7);
  assert(!live_outputs[1].speed_publish);
  assert(live_outputs[1].x > -0.1f && live_outputs[1].x < 0.1f);
  assert(live_outputs[1].y > 6.9f && live_outputs[1].y < 7.1f);
  assert(std::isnan(live_outputs[1].corrected_speed));
  assert(std::isnan(live_outputs[1].corrected_speed_mph));
  assert(live_outputs[2].present);
  assert(live_outputs[2].target.distance == 9);
  assert(!live_outputs[2].speed_publish);
  assert(live_outputs[2].x > -0.1f && live_outputs[2].x < 0.1f);
  assert(live_outputs[2].y < -8.9f && live_outputs[2].y > -9.1f);
  assert(std::isnan(live_outputs[2].corrected_speed));
  assert(std::isnan(live_outputs[2].corrected_speed_mph));

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
