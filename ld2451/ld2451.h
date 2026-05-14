#pragma once

#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ld2451 {

class LD2451BluetoothSwitch;

class LD2451Component : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_target_count_sensor(sensor::Sensor *sensor) { this->target_count_sensor_ = sensor; }
  void set_alarm_binary_sensor(binary_sensor::BinarySensor *sensor) { this->alarm_binary_sensor_ = sensor; }
  void set_angle_sensor(sensor::Sensor *sensor) { this->angle_sensor_ = sensor; }
  void set_distance_sensor(sensor::Sensor *sensor) { this->distance_sensor_ = sensor; }
  void set_speed_sensor(sensor::Sensor *sensor) { this->speed_sensor_ = sensor; }
  void set_snr_sensor(sensor::Sensor *sensor) { this->snr_sensor_ = sensor; }
  void set_direction_text_sensor(text_sensor::TextSensor *sensor) { this->direction_text_sensor_ = sensor; }
  void set_bluetooth_switch(LD2451BluetoothSwitch *sw) { this->bluetooth_switch_ = sw; }
  void set_disable_bluetooth_on_boot(bool value) { this->disable_bluetooth_on_boot_ = value; }
  bool set_bluetooth_enabled(bool enabled);

 protected:
  struct ParsedTarget {
    int angle;
    uint8_t distance;
    uint8_t direction;
    uint8_t speed;
    uint8_t snr;
  };

  bool extract_frame_();
  static bool parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                             ParsedTarget &first_target);
  void publish_frame_(uint8_t target_count, bool alarm, const ParsedTarget &first_target, bool has_target);
  bool send_command_frame_(uint8_t cmd, const uint8_t *payload, size_t payload_len);
  bool read_ack_frame_(uint8_t expected_cmd, uint16_t timeout_ms);
  bool enter_config_();
  bool exit_config_();
  bool set_bluetooth_enabled_internal_(bool enabled);

  friend bool ld2451_test_parse_payload(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                                        ParsedTarget &first_target);

  std::vector<uint8_t> rx_buffer_;
  uint32_t parsed_frames_{0};
  uint32_t empty_frames_{0};
  uint32_t last_empty_hint_ms_{0};

  sensor::Sensor *target_count_sensor_{nullptr};
  binary_sensor::BinarySensor *alarm_binary_sensor_{nullptr};
  sensor::Sensor *angle_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
  sensor::Sensor *snr_sensor_{nullptr};
  text_sensor::TextSensor *direction_text_sensor_{nullptr};
  LD2451BluetoothSwitch *bluetooth_switch_{nullptr};
  bool disable_bluetooth_on_boot_{false};
  bool bluetooth_enabled_state_{true};
};

class LD2451BluetoothSwitch : public switch_::Switch {
 public:
  void set_parent(LD2451Component *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  LD2451Component *parent_{nullptr};
};

}  // namespace ld2451
}  // namespace esphome
