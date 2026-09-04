# LD2451 ESPHome External Component

ESPHome external component for the HLK-LD2451 radar over UART (115200 baud).

This component parses LD2451 live data frames and exposes key values as ESPHome entities.

## Features

| Capability        | Details                                                                                                                                           |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| Frame parsing     | Parses LD2451 data frames (`F4 F3 F2 F1 ... F8 F7 F6 F5`)                                                                                         |
| Published data    | target count, vehicle detected, nearest target angle/distance/speed/speed_mph/SNR/direction, plus live `target_1_*` / `target_2_*` / `target_3_*` frame-order slots with `x`, `y`, `angle`, `distance`, `speed`, `speed_mph`, `snr`, and `direction`, plus slot-local rolling `*_min` / `*_max` / `*_avg` summary sensors for `distance`, `speed`, `speed_mph`, and `snr` |
| Direction mapping | `0x00` => `Approaching`, `0x01` => `Moving away`, idle => `None`                                                                                  |

## Installation

Add this component from GitHub in your ESPHome YAML.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/poolski/esphome-components
    refresh: 1d
    components: [ld2451]
```

`refresh` sets how often ESPHome refreshes the GitHub source cache. `1d` is a good default.
Use a shorter value while developing and a longer value if you want fewer update checks.

Equivalent shorthand:

```yaml
external_components:
  - source: github://poolski/esphome-components@main
    refresh: 1d
    components: [ld2451]
```

## Minimal Example

```yaml
esphome:
  name: radar-node

esp32:
  board: esp32dev

logger:
  level: DEBUG
api:
ota:
  platform: esphome
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

external_components:
  - source:
      type: git
      url: https://github.com/poolski/esphome-components
    refresh: 1d
    components: [ld2451]

uart:
  id: uart_bus
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200

ld2451:
  id: radar
  uart_id: uart_bus
  target_count:
    name: "LD2451 Target Count"
  vehicle_detected:
    name: "LD2451 Vehicle Detected"
  angle:
    name: "LD2451 Angle"
  distance:
    name: "LD2451 Distance"
  speed:
    name: "LD2451 Speed"
  speed_mph:
    name: "LD2451 Speed MPH"
  snr:
    name: "LD2451 SNR"
  direction:
    name: "LD2451 Direction"
  speed_publish_max_abs_angle: 65
  target_1_angle:
    name: "LD2451 Target 1 Angle"
  target_1_x:
    name: "LD2451 Target 1 X"
  target_1_y:
    name: "LD2451 Target 1 Y"
  target_1_distance:
    name: "LD2451 Target 1 Distance"
  target_1_speed:
    name: "LD2451 Target 1 Speed"
  target_1_speed_mph:
    name: "LD2451 Target 1 Speed MPH"
  target_1_snr:
    name: "LD2451 Target 1 SNR"
  target_1_direction:
    name: "LD2451 Target 1 Direction"
  target_1_distance_min:
    name: "LD2451 Target 1 Distance Min"
  target_1_distance_max:
    name: "LD2451 Target 1 Distance Max"
  target_1_distance_avg:
    name: "LD2451 Target 1 Distance Avg"
  target_1_speed_min:
    name: "LD2451 Target 1 Speed Min"
  target_1_speed_max:
    name: "LD2451 Target 1 Speed Max"
  target_1_speed_avg:
    name: "LD2451 Target 1 Speed Avg"
  target_1_speed_mph_min:
    name: "LD2451 Target 1 Speed MPH Min"
  target_1_speed_mph_max:
    name: "LD2451 Target 1 Speed MPH Max"
  target_1_speed_mph_avg:
    name: "LD2451 Target 1 Speed MPH Avg"
  target_1_snr_min:
    name: "LD2451 Target 1 SNR Min"
  target_1_snr_max:
    name: "LD2451 Target 1 SNR Max"
  target_1_snr_avg:
    name: "LD2451 Target 1 SNR Avg"
  # Repeat the same pattern for target_2_* and target_3_*
```

## Configuration

`ld2451:` supports:

| Key                           | Type              | Required | Default | Notes                                                                                                   |
| ----------------------------- | ----------------- | -------- | ------- | ------------------------------------------------------------------------------------------------------- |
| `id`                          | component ID      | yes      | -       | -                                                                                                       |
| `uart_id`                     | UART ID           | yes      | -       | -                                                                                                       |
| `min_distance`                | int (0..100 m)    | no       | `0`     | Software filter: targets closer than this are not published. `0` disables it.                           |
| `speed_publish_max_abs_angle` | int (0..90 deg)   | no       | `0`     | Software confidence gate: a target beyond this absolute angle is treated as absent. `0` disables it.    |
| `speed_correction`            | float multiplier  | no       | `1.0`   | Applied to the published `speed` (and `speed_mph`, which derives from the same corrected value).        |

These are all software-side filters applied inside ESPHome. Device-stored settings such as `max_distance`, `trigger_count`, and `min_snr` are not configurable here; set those from the LD2451 mobile app.

It exposes the following sensors:

| Key                | Type            | Required | Notes                                                                         |
| ------------------ | --------------- | -------- | ----------------------------------------------------------------------------- |
| `target_count`     | `sensor`        | no       | Number of targets in the current frame                                        |
| `vehicle_detected` | `binary_sensor` | no       | `ON` when device alarm fires (trigger_count met); `OFF` after no_target_delay |
| `angle`            | `sensor`        | no       | Degrees; negative = left of sensor axis                                       |
| `distance`         | `sensor`        | no       | Metres to nearest qualifying target                                           |
| `speed`            | `sensor`        | no       | km/h (after `speed_correction`)                                               |
| `speed_mph`        | `sensor`        | no       | mph (after `speed_correction`)                                                |
| `snr`              | `sensor`        | no       | Signal-to-noise ratio (0..255)                                                |
| `direction`        | `text_sensor`   | no       | `Approaching`, `Moving away`, or `None`                                       |
| `target_1_*` / `target_2_*` / `target_3_*` | `sensor` / `text_sensor` | no | Live frame-order target slots with `x`, `y`, `angle`, `distance`, `speed`, `speed_mph`, `snr`, and `direction` |
| `target_[1-3]_{distance,speed,speed_mph,snr}_{min,max,avg}` | `sensor` | no | Slot-local rolling summaries; reset to `NaN` when the slot disappears |

UART validation is enforced for:

| Validation     | Requirement |
| -------------- | ----------- |
| UART baud rate | `115200`    |
| RX pin         | required    |

## Notes

| Topic              | Detail                                                                                                                                                                                                              |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Target publishing  | Current implementation publishes the nearest qualifying target per frame, exposes the first three raw targets as live frame-order slots, and keeps rolling min/max/avg stats for `distance`, `speed`, `speed_mph`, and `snr` within each live slot |
| No-target behavior | After `no_target_delay`, nearest-target fields reset to `0`, live target slots reset to `NaN` / `None`, and `vehicle_detected` resets to `OFF`                                                                       |
| Runtime settings   | Device-side parameters such as `max_distance`, `trigger_count`, and `min_snr` stay on the radar itself; ESPHome only consumes live frames.                                                                        |
| Distance filtering | `min_distance` is software-side only: targets closer than this value are suppressed, but a farther target from the same frame may still publish. `max_distance` is device-side only: the device enforces it, so ESPHome publishes whatever the device reports.                  |
| Speed filtering    | `speed_publish_max_abs_angle` is a software-side confidence gate for target publication. When a target fails this gate, it is treated as absent and none of its live entity updates are published. |
| Speed correction   | `speed_correction` is software-side only: published speed is multiplied by this value; `speed_mph` is derived from the same corrected value                                                               |

### Home Assistant automation trigger

Use `vehicle_detected` (`off` -> `on`) as the trigger in HA automations. The speed, distance, angle, SNR, and direction sensors update on qualifying detections and retain their last observed values after detection ends.

## Fast Host Tests

Run pure C++ host tests without firmware compile:

```bash
components/ld2451/tests_host/run_host_tests.sh
```

## Debug Logging

Set logger level to `DEBUG` (or more verbose) to see runtime parser diagnostics:

```yaml
logger:
  level: DEBUG
```

At `DEBUG`, the component logs:

| Log type          | Details                                                                      |
| ----------------- | ---------------------------------------------------------------------------- |
| Parsed telemetry  | decoded target values (`targets`, first target angle, distance, speed, direction, snr) |
| Telemetry hints   | periodic hints when the sensor is sending data but no target payload is present |

Important for pre-install bench checks: you may need large, deliberate movement to trigger detection
(for example, walking or running toward the sensor, or broad hand/body motion). Small or static motion often does not
trigger frames, especially with restrictive sensor-side settings.

## Troubleshooting: No Detections

1. Confirm UART and component setup
   - `baud_rate: 115200`
   - correct `rx_pin` wiring from sensor TX
   - `external_components` uses `source: github://poolski/esphome-components@main`
2. Turn on debug logs and watch runtime telemetry
   - set `logger.level: DEBUG`
   - look for LD2451 debug lines (parsed telemetry, heartbeat frames, and no-target hints)
3. Use deliberate movement for bench validation before installation
   - move directly toward sensor with larger motion (walk/run toward it)
   - if needed, test with broad hand/body movement at close range

## Component CI

This repository is intended to host multiple components. CI runs tests per component directory
(not as one monolithic repo check).

Local test command for this component:

```bash
../../scripts/test-component.sh components/ld2451
```
