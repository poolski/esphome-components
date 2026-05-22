#pragma once

#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "ack_codec.h"
#include "types.h"

namespace esphome {
namespace ld2451 {

class LD2451Component;

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
  void set_angle_sensor(sensor::Sensor *sensor) { this->angle_sensor_ = sensor; }
  void set_distance_sensor(sensor::Sensor *sensor) { this->distance_sensor_ = sensor; }
  void set_speed_sensor(sensor::Sensor *sensor) { this->speed_sensor_ = sensor; }
  void set_snr_sensor(sensor::Sensor *sensor) { this->snr_sensor_ = sensor; }
  void set_direction_text_sensor(text_sensor::TextSensor *sensor) { this->direction_text_sensor_ = sensor; }

 protected:
  bool extract_frame_();
  bool read_firmware_version_(FirmwareVersionInfo &out);
  static constexpr uint32_t COMMAND_ACK_TIMEOUT_MS = 400;
  static constexpr uint16_t MAX_ACK_PAYLOAD_LEN = 64;
  bool send_command_wait_ack_(uint16_t command, const std::vector<uint8_t> &value, std::vector<uint8_t> *ret = nullptr,
                              uint32_t timeout_ms = COMMAND_ACK_TIMEOUT_MS);
  static bool parse_payload_(const std::vector<uint8_t> &payload, uint8_t &target_count, ParsedTarget &first_target);
  void publish_frame_(uint8_t target_count, const ParsedTarget &first_target, bool has_target);

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_empty_hint_ms_{0};
  uint32_t last_rx_activity_log_ms_{0};
  SensorSettings desired_{};

  sensor::Sensor *target_count_sensor_{nullptr};
  binary_sensor::BinarySensor *vehicle_detected_binary_sensor_{nullptr};
  sensor::Sensor *angle_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
  sensor::Sensor *snr_sensor_{nullptr};
  text_sensor::TextSensor *direction_text_sensor_{nullptr};
  bool detection_active_{false};
  bool idle_published_{false};
  uint32_t last_detection_ms_{0};
};

}  // namespace ld2451
}  // namespace esphome
