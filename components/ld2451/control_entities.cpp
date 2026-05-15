#include "control_entities.h"

#include <cmath>

#include "config_state.h"
#include "esphome/core/log.h"
#include "ld2451.h"

namespace esphome {
namespace ld2451 {

static const char *const TAG = "ld2451";

void LD2451MaxDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_max_distance(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_max_distance());
}

void LD2451MinDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_min_distance(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_min_distance());
}

void LD2451MinSpeedNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_min_speed(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_min_speed());
}

void LD2451NoTargetDelayNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_no_target_delay(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_no_target_delay());
}

void LD2451TriggerCountNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_trigger_count(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_trigger_count());
}

void LD2451MinSnrNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  int requested = static_cast<int>(lroundf(value));
  if (requested != 0 && requested < 3) {
    ESP_LOGW(TAG, "Coercing min_snr=%d to 0; allowed values are 0 or 3..8", requested);
    requested = 0;
  }
  this->parent_->set_min_snr(requested);
  this->publish_state(this->parent_->get_min_snr());
}

void LD2451SnrThresholdNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_snr_threshold(static_cast<int>(lroundf(value)));
  this->publish_state(this->parent_->get_snr_threshold());
}

void LD2451SpeedCorrectionNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_speed_correction(value);
  this->publish_state(this->parent_->get_speed_correction());
}

void LD2451DetectionDirectionSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->set_detection_direction_option(value);
  this->publish_state(this->parent_->get_detection_direction_option());
}

}  // namespace ld2451
}  // namespace esphome
