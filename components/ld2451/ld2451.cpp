#include "ld2451.h"

#include "config_state.h"
#include "target_publisher.h"
#include <limits>
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

static size_t count_present_targets(const std::array<LiveTargetOutput, kLiveTargetSlotCount> &outputs);

void LD2451Component::set_live_target_angle_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].angle = sensor;
  }
}

void LD2451Component::set_live_target_x_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].x = sensor;
  }
}

void LD2451Component::set_live_target_y_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].y = sensor;
  }
}

void LD2451Component::set_live_target_distance_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].distance = sensor;
  }
}

void LD2451Component::set_live_target_speed_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].speed = sensor;
  }
}

void LD2451Component::set_live_target_speed_mph_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].speed_mph = sensor;
  }
}

void LD2451Component::set_live_target_snr_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].snr = sensor;
  }
}

void LD2451Component::set_live_target_direction_text_sensor(uint8_t slot, text_sensor::TextSensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_sensors_[slot].direction = sensor;
  }
}

void LD2451Component::set_live_target_distance_min_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].distance.min = sensor;
  }
}

void LD2451Component::set_live_target_distance_max_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].distance.max = sensor;
  }
}

void LD2451Component::set_live_target_distance_avg_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].distance.avg = sensor;
  }
}

void LD2451Component::set_live_target_speed_min_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed.min = sensor;
  }
}

void LD2451Component::set_live_target_speed_max_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed.max = sensor;
  }
}

void LD2451Component::set_live_target_speed_avg_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed.avg = sensor;
  }
}

void LD2451Component::set_live_target_speed_mph_min_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed_mph.min = sensor;
  }
}

void LD2451Component::set_live_target_speed_mph_max_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed_mph.max = sensor;
  }
}

void LD2451Component::set_live_target_speed_mph_avg_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].speed_mph.avg = sensor;
  }
}

void LD2451Component::set_live_target_snr_min_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].snr.min = sensor;
  }
}

void LD2451Component::set_live_target_snr_max_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].snr.max = sensor;
  }
}

void LD2451Component::set_live_target_snr_avg_sensor(uint8_t slot, sensor::Sensor *sensor) {
  if (slot < kLiveTargetSlotCount) {
    this->live_target_summary_sensors_[slot].snr.avg = sensor;
  }
}

void LD2451Component::setup() {
  ESP_LOGI(TAG, "Runtime configuration via ESPHome is disabled");
  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(false);
  }
  this->idle_published_ = true;
}

void LD2451Component::loop() {
  const uint32_t now = millis();
  std::vector<uint8_t> chunk;
  while (this->available()) {
    chunk.push_back(this->read());
  }
  const size_t bytes_read = chunk.size();
  if (bytes_read > 0) {
    this->frame_parser_.push(chunk.data(), chunk.size());
  }

  bool any_frame = false;
  ParsedFrame frame;
  while (this->frame_parser_.pop(frame)) {
    this->handle_frame_(frame);
    any_frame = true;
  }
  if (bytes_read > 0 && !any_frame) {
    if (now - this->last_rx_activity_log_ms_ > 5000) {
      ESP_LOGD(TAG, "RX activity: read=%u bytes awaiting a complete frame", static_cast<unsigned int>(bytes_read));
      this->last_rx_activity_log_ms_ = now;
    }
  }
}

void LD2451Component::dump_config() {
  ESP_LOGCONFIG(TAG, "LD2451:");
  ESP_LOGCONFIG(TAG, "  Runtime config:      disabled in ESPHome");
  ESP_LOGCONFIG(TAG, "  Min Distance:        %u m (software filter)", this->desired_.min_distance);
  ESP_LOGCONFIG(TAG, "  Speed Publish Max Angle: %u deg (software filter)", this->desired_.speed_publish_max_abs_angle);
  ESP_LOGCONFIG(TAG, "  Speed Correction:    %.2fx (software only)", this->desired_.speed_correction);
  LOG_SENSOR("  ", "Target Count", this->target_count_sensor_);
  LOG_BINARY_SENSOR("  ", "Vehicle Detected", this->vehicle_detected_binary_sensor_);
  LOG_SENSOR("  ", "Angle", this->angle_sensor_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Speed", this->speed_sensor_);
  LOG_SENSOR("  ", "Speed MPH", this->speed_mph_sensor_);
  LOG_SENSOR("  ", "SNR", this->snr_sensor_);
  LOG_TEXT_SENSOR("  ", "Direction", this->direction_text_sensor_);
  for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
    ESP_LOGCONFIG(TAG, "  Target %u:", static_cast<unsigned int>(i + 1));
    LOG_SENSOR("    ", "X", this->live_target_sensors_[i].x);
    LOG_SENSOR("    ", "Y", this->live_target_sensors_[i].y);
    LOG_SENSOR("    ", "Angle", this->live_target_sensors_[i].angle);
    LOG_SENSOR("    ", "Distance", this->live_target_sensors_[i].distance);
    LOG_SENSOR("    ", "Speed", this->live_target_sensors_[i].speed);
    LOG_SENSOR("    ", "Speed MPH", this->live_target_sensors_[i].speed_mph);
    LOG_SENSOR("    ", "SNR", this->live_target_sensors_[i].snr);
    LOG_TEXT_SENSOR("    ", "Direction", this->live_target_sensors_[i].direction);
    LOG_SENSOR("    ", "Distance Min", this->live_target_summary_sensors_[i].distance.min);
    LOG_SENSOR("    ", "Distance Max", this->live_target_summary_sensors_[i].distance.max);
    LOG_SENSOR("    ", "Distance Avg", this->live_target_summary_sensors_[i].distance.avg);
    LOG_SENSOR("    ", "Speed Min", this->live_target_summary_sensors_[i].speed.min);
    LOG_SENSOR("    ", "Speed Max", this->live_target_summary_sensors_[i].speed.max);
    LOG_SENSOR("    ", "Speed Avg", this->live_target_summary_sensors_[i].speed.avg);
    LOG_SENSOR("    ", "Speed MPH Min", this->live_target_summary_sensors_[i].speed_mph.min);
    LOG_SENSOR("    ", "Speed MPH Max", this->live_target_summary_sensors_[i].speed_mph.max);
    LOG_SENSOR("    ", "Speed MPH Avg", this->live_target_summary_sensors_[i].speed_mph.avg);
    LOG_SENSOR("    ", "SNR Min", this->live_target_summary_sensors_[i].snr.min);
    LOG_SENSOR("    ", "SNR Max", this->live_target_summary_sensors_[i].snr.max);
    LOG_SENSOR("    ", "SNR Avg", this->live_target_summary_sensors_[i].snr.avg);
  }
}

float LD2451Component::get_setup_priority() const { return setup_priority::DATA; }

void LD2451Component::handle_frame_(const ParsedFrame &frame) {
  const uint8_t target_count = frame.target_count;
  const bool alarm = frame.alarm;
  const bool has_targets = frame.has_target;
  const std::vector<ParsedTarget> &targets = frame.targets;
  const auto live_targets = build_live_target_outputs(this->desired_, targets);
  const size_t confident_target_count = count_present_targets(live_targets);

  if (!has_targets) {
    const uint32_t now = millis();
    if (now - this->last_empty_hint_ms_ > 5000) {
      ESP_LOGD(TAG, "No target payload received. Sensor telemetry is active.");
      this->last_empty_hint_ms_ = now;
    }
  } else if (confident_target_count == 0 && !targets.empty()) {
    const std::string reason = confidence_filter_reason(this->desired_, targets.front());
    ESP_LOGD(TAG, "Ignored target due to low confidence. [%s]", reason.c_str());
  } else {
    const LiveTargetOutput *first_present = nullptr;
    for (const auto &candidate : live_targets) {
      if (candidate.present) {
        first_present = &candidate;
        break;
      }
    }
    if (first_present != nullptr) {
      ESP_LOGD(TAG,
               "Parsed telemetry: targets=%u first_angle=%ddeg first_dist=%um first_speed=%ukm/h first_dir_raw=0x%02X "
               "first_dir=%s first_snr=%u",
               target_count, first_present->target.angle, first_present->target.distance, first_present->target.speed,
               first_present->target.direction, direction_label(first_present->target.direction),
               first_present->target.snr);
    }
  }

  this->publish_frame_(target_count, targets, alarm, has_targets);
}

void LD2451Component::clear_live_target_slot_(uint8_t slot) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  const auto &sensors = this->live_target_sensors_[slot];
  if (sensors.x != nullptr) {
    sensors.x->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.y != nullptr) {
    sensors.y->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.angle != nullptr) {
    sensors.angle->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.distance != nullptr) {
    sensors.distance->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.speed != nullptr) {
    sensors.speed->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.speed_mph != nullptr) {
    sensors.speed_mph->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.snr != nullptr) {
    sensors.snr->publish_state(std::numeric_limits<float>::quiet_NaN());
  }
  if (sensors.direction != nullptr) {
    sensors.direction->publish_state("None");
  }
}

static void publish_stat_sensor(sensor::Sensor *sensor, float value) {
  if (sensor != nullptr) {
    sensor->publish_state(value);
  }
}

static void publish_stat_triplet(const LiveTargetStatSensors &sensors, const RollingStat &stats) {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  if (!stats.has_samples()) {
    publish_stat_sensor(sensors.min, nan);
    publish_stat_sensor(sensors.max, nan);
    publish_stat_sensor(sensors.avg, nan);
    return;
  }

  publish_stat_sensor(sensors.min, stats.min());
  publish_stat_sensor(sensors.max, stats.max());
  publish_stat_sensor(sensors.avg, stats.average());
}

void LD2451Component::publish_live_target_summary_slot_(uint8_t slot) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  const auto &sensors = this->live_target_summary_sensors_[slot];
  const auto &stats = this->live_target_stats_[slot];
  publish_stat_triplet(sensors.distance, stats.distance);
  publish_stat_triplet(sensors.speed, stats.speed);
  publish_stat_triplet(sensors.speed_mph, stats.speed_mph);
  publish_stat_triplet(sensors.snr, stats.snr);
}

void LD2451Component::clear_live_target_summary_slot_(uint8_t slot) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  const auto &sensors = this->live_target_summary_sensors_[slot];
  const float nan = std::numeric_limits<float>::quiet_NaN();
  publish_stat_sensor(sensors.distance.min, nan);
  publish_stat_sensor(sensors.distance.max, nan);
  publish_stat_sensor(sensors.distance.avg, nan);
  publish_stat_sensor(sensors.speed.min, nan);
  publish_stat_sensor(sensors.speed.max, nan);
  publish_stat_sensor(sensors.speed.avg, nan);
  publish_stat_sensor(sensors.speed_mph.min, nan);
  publish_stat_sensor(sensors.speed_mph.max, nan);
  publish_stat_sensor(sensors.speed_mph.avg, nan);
  publish_stat_sensor(sensors.snr.min, nan);
  publish_stat_sensor(sensors.snr.max, nan);
  publish_stat_sensor(sensors.snr.avg, nan);
}

static size_t count_present_targets(const std::array<LiveTargetOutput, kLiveTargetSlotCount> &outputs) {
  size_t count = 0;
  for (const auto &output : outputs) {
    if (output.present) {
      count++;
    }
  }
  return count;
}

void LD2451Component::publish_live_target_slot_(uint8_t slot, const LiveTargetOutput &output) {
  if (slot >= kLiveTargetSlotCount) {
    return;
  }

  if (!output.present) {
    this->clear_live_target_slot_(slot);
    return;
  }

  const auto &sensors = this->live_target_sensors_[slot];
  if (sensors.x != nullptr) {
    sensors.x->publish_state(output.x);
  }
  if (sensors.y != nullptr) {
    sensors.y->publish_state(output.y);
  }
  if (sensors.angle != nullptr) {
    sensors.angle->publish_state(output.target.angle);
  }
  if (sensors.distance != nullptr) {
    sensors.distance->publish_state(output.target.distance);
  }
  if (sensors.speed != nullptr) {
    sensors.speed->publish_state(output.corrected_speed);
  }
  if (sensors.speed_mph != nullptr) {
    sensors.speed_mph->publish_state(output.corrected_speed_mph);
  }
  if (sensors.snr != nullptr) {
    sensors.snr->publish_state(output.target.snr);
  }
  if (sensors.direction != nullptr) {
    sensors.direction->publish_state(direction_label(output.target.direction));
  }
}

void LD2451Component::publish_frame_(uint8_t target_count, const std::vector<ParsedTarget> &targets, bool alarm,
                                     bool has_targets) {
  const auto live_targets = build_live_target_outputs(this->desired_, targets);
  const size_t confident_target_count = count_present_targets(live_targets);

  if (this->target_count_sensor_ != nullptr) {
    const float new_count = static_cast<float>(confident_target_count);
    if (!this->target_count_sensor_->has_state() || this->target_count_sensor_->state != new_count) {
      this->target_count_sensor_->publish_state(new_count);
    }
  }

  const uint32_t now = millis();
  const auto publish_idle_reset = [this]() {
    if (this->vehicle_detected_binary_sensor_ != nullptr) {
      this->vehicle_detected_binary_sensor_->publish_state(false);
    }
    this->detection_active_ = false;
    this->idle_published_ = true;
  };

  const auto maybe_publish_idle_reset = [this, now, &publish_idle_reset]() {
    if (should_publish_idle_reset(this->detection_active_, this->idle_published_, now, this->last_detection_ms_,
                                  this->desired_.no_target_delay)) {
      publish_idle_reset();
    }
  };

  if (!has_targets || confident_target_count == 0) {
    for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
      this->live_target_stats_[i].reset();
      this->clear_live_target_slot_(i);
      this->clear_live_target_summary_slot_(i);
    }
    maybe_publish_idle_reset();
    return;
  }

  for (uint8_t i = 0; i < kLiveTargetSlotCount; i++) {
    if (live_targets[i].present) {
      this->live_target_stats_[i].ingest(live_targets[i]);
    } else {
      this->live_target_stats_[i].reset();
    }
    this->publish_live_target_slot_(i, live_targets[i]);
    if (live_targets[i].present) {
      this->publish_live_target_summary_slot_(i);
    } else {
      this->clear_live_target_summary_slot_(i);
    }
  }

  if (this->vehicle_detected_binary_sensor_ != nullptr) {
    this->vehicle_detected_binary_sensor_->publish_state(alarm);
  }

  this->last_detection_ms_ = now;
  this->detection_active_ = true;
  this->idle_published_ = false;

  if (targets.size() > kLiveTargetSlotCount) {
    ESP_LOGD(TAG, "Frame contains %u targets; exposing first %u live slots", static_cast<unsigned int>(targets.size()),
             static_cast<unsigned int>(kLiveTargetSlotCount));
  }

  ParsedTarget nearest_target{};
  const bool has_nearest_target = select_nearest_qualifying_target(this->desired_, targets, nearest_target);
  if (!has_nearest_target) {
    ESP_LOGD(TAG, "No qualifying target found for nearest-target entities (min_distance=%u)",
             this->desired_.min_distance);
  } else {
    const TargetOutput output = compute_target_output(this->desired_, nearest_target);
    // vehicle_detected fires only when the device alarm flag is set (trigger_count met).
    // Nearest target values are published for the closest qualifying target.
    if (this->vehicle_detected_binary_sensor_ != nullptr) {
      this->vehicle_detected_binary_sensor_->publish_state(alarm);
    }

    if (this->angle_sensor_ != nullptr) {
      this->angle_sensor_->publish_state(nearest_target.angle);
    }
    if (this->distance_sensor_ != nullptr) {
      this->distance_sensor_->publish_state(nearest_target.distance);
    }
    if (this->speed_sensor_ != nullptr) {
      this->speed_sensor_->publish_state(output.corrected_speed);
    }
    if (this->speed_mph_sensor_ != nullptr) {
      this->speed_mph_sensor_->publish_state(output.corrected_speed_mph);
    }
    if (this->snr_sensor_ != nullptr) {
      this->snr_sensor_->publish_state(nearest_target.snr);
    }
    if (this->direction_text_sensor_ != nullptr) {
      this->direction_text_sensor_->publish_state(direction_label(nearest_target.direction));
    }
  }

  if (confident_target_count < target_count) {
    ESP_LOGD(TAG, "Filtered %u low-confidence targets from frame",
             static_cast<unsigned int>(target_count - confident_target_count));
  }
}

}  // namespace ld2451
}  // namespace esphome
