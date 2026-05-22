#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "types.h"
#include "target_publisher.h"

namespace esphome {
namespace ld2451 {

class LD2451Component;

struct LiveTargetSensors {
  sensor::Sensor *x{nullptr};
  sensor::Sensor *y{nullptr};
  sensor::Sensor *angle{nullptr};
  sensor::Sensor *distance{nullptr};
  sensor::Sensor *speed{nullptr};
  sensor::Sensor *speed_mph{nullptr};
  sensor::Sensor *snr{nullptr};
  text_sensor::TextSensor *direction{nullptr};
};

class LD2451Component : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_target_count_sensor(sensor::Sensor *sensor) { this->target_count_sensor_ = sensor; }
  void set_vehicle_detected_binary_sensor(binary_sensor::BinarySensor *sensor) {
    this->vehicle_detected_binary_sensor_ = sensor;
  }
  void set_speed_publish_min_snr(uint8_t value) { this->desired_.speed_publish_min_snr = value; }
  void set_speed_publish_max_abs_angle(uint8_t value) { this->desired_.speed_publish_max_abs_angle = value; }
  void set_angle_sensor(sensor::Sensor *sensor) { this->angle_sensor_ = sensor; }
  void set_distance_sensor(sensor::Sensor *sensor) { this->distance_sensor_ = sensor; }
  void set_speed_sensor(sensor::Sensor *sensor) { this->speed_sensor_ = sensor; }
  void set_speed_mph_sensor(sensor::Sensor *sensor) { this->speed_mph_sensor_ = sensor; }
  void set_snr_sensor(sensor::Sensor *sensor) { this->snr_sensor_ = sensor; }
  void set_direction_text_sensor(text_sensor::TextSensor *sensor) { this->direction_text_sensor_ = sensor; }
  void set_live_target_x_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_y_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_angle_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_distance_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_speed_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_speed_mph_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_snr_sensor(uint8_t slot, sensor::Sensor *sensor);
  void set_live_target_direction_text_sensor(uint8_t slot, text_sensor::TextSensor *sensor);

 protected:
  bool extract_frame_();
  bool parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                      std::vector<ParsedTarget> &targets);
  void publish_frame_(uint8_t target_count, const std::vector<ParsedTarget> &targets, bool alarm, bool has_targets);
  void publish_live_target_slot_(uint8_t slot, const LiveTargetOutput &output);
  void clear_live_target_slot_(uint8_t slot);

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_empty_hint_ms_{0};
  uint32_t last_rx_activity_log_ms_{0};
  SensorSettings desired_{};

  sensor::Sensor *target_count_sensor_{nullptr};
  binary_sensor::BinarySensor *vehicle_detected_binary_sensor_{nullptr};
  sensor::Sensor *angle_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
  sensor::Sensor *speed_mph_sensor_{nullptr};
  sensor::Sensor *snr_sensor_{nullptr};
  text_sensor::TextSensor *direction_text_sensor_{nullptr};
  std::array<LiveTargetSensors, kLiveTargetSlotCount> live_target_sensors_{};
  bool detection_active_{false};
  bool idle_published_{false};
  uint32_t last_detection_ms_{0};
};

}  // namespace ld2451
}  // namespace esphome
