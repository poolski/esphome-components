#pragma once

#include <string>

#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
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

class LD2451AppSnrThresholdNumber : public number::Number, public Parented<LD2451Component> {
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

}  // namespace ld2451
}  // namespace esphome
