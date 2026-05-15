#pragma once

#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ld2451 {

class LD2451Component;

class LD2451MaxDistanceNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451MinDistanceNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451MinSpeedNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451NoTargetDelayNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451TriggerCountNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451MinSnrNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451SpeedCorrectionNumber : public number::Number, public Parented<LD2451Component> {
 public:
  void control(float value) override;
};

class LD2451DetectionDirectionSelect : public select::Select, public Parented<LD2451Component> {
 public:
  void control(const std::string &value) override;
};

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

  void set_max_distance(int value) {
    this->desired_.max_distance = static_cast<uint8_t>(value);
    if (this->desired_.min_distance > this->desired_.max_distance) {
      this->desired_.min_distance = this->desired_.max_distance;
    }
    this->config_dirty_ = true;
  }
  void set_min_distance(int value) {
    this->desired_.min_distance = static_cast<uint8_t>(value);
    if (this->desired_.min_distance > this->desired_.max_distance) {
      this->desired_.max_distance = this->desired_.min_distance;
      this->config_dirty_ = true;
    }
  }
  void set_min_speed(int value) {
    this->desired_.min_speed = static_cast<uint8_t>(value);
    this->config_dirty_ = true;
  }
  void set_detection_direction(int value) {
    this->desired_.detection_direction = static_cast<uint8_t>(value);
    this->config_dirty_ = true;
  }
  void set_no_target_delay(int value) {
    this->desired_.no_target_delay = static_cast<uint8_t>(value);
    this->config_dirty_ = true;
  }
  void set_trigger_count(int value) {
    this->desired_.trigger_count = static_cast<uint8_t>(value);
    this->config_dirty_ = true;
  }
  void set_min_snr(int value) {
    this->desired_.min_snr = static_cast<uint8_t>(value);
    this->config_dirty_ = true;
  }
  void set_speed_correction(float value) {
    this->desired_.speed_correction = value;
  }
  void set_detection_direction_option(const std::string &value) {
    this->set_detection_direction(this->detection_direction_from_option_(value));
  }

  uint8_t get_max_distance() const { return this->desired_.max_distance; }
  uint8_t get_min_distance() const { return this->desired_.min_distance; }
  uint8_t get_min_speed() const { return this->desired_.min_speed; }
  uint8_t get_no_target_delay() const { return this->desired_.no_target_delay; }
  uint8_t get_trigger_count() const { return this->desired_.trigger_count; }
  uint8_t get_min_snr() const { return this->desired_.min_snr; }
  float get_speed_correction() const { return this->desired_.speed_correction; }
  const char *get_detection_direction_option() const {
    return this->detection_direction_to_option_(this->desired_.detection_direction);
  }

  void set_max_distance_number(LD2451MaxDistanceNumber *n) { this->max_distance_number_ = n; }
  void set_min_distance_number(LD2451MinDistanceNumber *n) { this->min_distance_number_ = n; }
  void set_min_speed_number(LD2451MinSpeedNumber *n) { this->min_speed_number_ = n; }
  void set_no_target_delay_number(LD2451NoTargetDelayNumber *n) { this->no_target_delay_number_ = n; }
  void set_trigger_count_number(LD2451TriggerCountNumber *n) { this->trigger_count_number_ = n; }
  void set_min_snr_number(LD2451MinSnrNumber *n) { this->min_snr_number_ = n; }
  void set_speed_correction_number(LD2451SpeedCorrectionNumber *n) { this->speed_correction_number_ = n; }
  void set_detection_direction_select(LD2451DetectionDirectionSelect *s) { this->detection_direction_select_ = s; }

 protected:
  struct RuntimeConfig {
    uint8_t max_distance{100};
    uint8_t min_distance{0};
    uint8_t min_speed{0};
    uint8_t detection_direction{2};
    uint8_t no_target_delay{0};
    uint8_t trigger_count{1};
    uint8_t min_snr{0};
    float speed_correction{1.0f};
  };

  struct ParsedTarget {
    int angle;
    uint8_t distance;
    uint8_t direction;
    uint8_t speed;
    uint8_t snr;
  };

  bool extract_frame_();
  bool apply_runtime_config_();
  bool enter_config_mode_();
  bool exit_config_mode_();
  bool write_target_detection_params_();
  bool write_sensitivity_params_();
  void refresh_runtime_entities_();
  static constexpr uint32_t COMMAND_ACK_TIMEOUT_MS = 120;
  static constexpr uint16_t MAX_ACK_PAYLOAD_LEN = 64;
  static uint8_t detection_direction_from_option_(const std::string &value);
  static const char *detection_direction_to_option_(uint8_t value);
  bool send_command_wait_ack_(uint16_t command, const std::vector<uint8_t> &value, std::vector<uint8_t> *ret = nullptr,
                              uint32_t timeout_ms = COMMAND_ACK_TIMEOUT_MS);
  bool read_exact_(uint8_t *dest, size_t len, uint32_t timeout_ms);
  static bool parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                             ParsedTarget &first_target);
  void publish_frame_(uint8_t target_count, bool alarm, const ParsedTarget &first_target, bool has_target);

  friend bool ld2451_test_parse_payload(const std::vector<uint8_t> &payload, uint8_t &target_count, bool &alarm,
                                        ParsedTarget &first_target);

  std::vector<uint8_t> rx_buffer_;
  uint32_t parsed_frames_{0};
  uint32_t empty_frames_{0};
  uint32_t short_payload_frames_{0};
  uint32_t last_empty_hint_ms_{0};
  uint32_t last_rx_activity_log_ms_{0};
  bool config_dirty_{false};
  bool config_in_flight_{false};
  uint32_t last_config_attempt_ms_{0};
  RuntimeConfig desired_{};
  RuntimeConfig applied_{};

  sensor::Sensor *target_count_sensor_{nullptr};
  binary_sensor::BinarySensor *alarm_binary_sensor_{nullptr};
  sensor::Sensor *angle_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
  sensor::Sensor *snr_sensor_{nullptr};
  text_sensor::TextSensor *direction_text_sensor_{nullptr};

  LD2451MaxDistanceNumber *max_distance_number_{nullptr};
  LD2451MinDistanceNumber *min_distance_number_{nullptr};
  LD2451MinSpeedNumber *min_speed_number_{nullptr};
  LD2451NoTargetDelayNumber *no_target_delay_number_{nullptr};
  LD2451TriggerCountNumber *trigger_count_number_{nullptr};
  LD2451MinSnrNumber *min_snr_number_{nullptr};
  LD2451SpeedCorrectionNumber *speed_correction_number_{nullptr};
  LD2451DetectionDirectionSelect *detection_direction_select_{nullptr};
};

}  // namespace ld2451
}  // namespace esphome
