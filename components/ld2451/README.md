# LD2451 ESPHome External Component

ESPHome external component for the HLK-LD2451 radar over UART (115200 baud).

This component parses LD2451 live data frames and exposes key values as ESPHome entities.

## Features

| Capability        | Details                                                                                                                                           |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| Frame parsing     | Parses LD2451 data frames (`F4 F3 F2 F1 ... F8 F7 F6 F5`)                                                                                         |
| Published data    | target count, vehicle detected, nearest target angle, nearest target distance, nearest target speed, nearest target SNR, nearest target direction |
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
  snr:
    name: "LD2451 SNR"
  direction:
    name: "LD2451 Direction"
```

## Configuration

`ld2451:` supports:

| Key       | Type         | Required | Notes |
| --------- | ------------ | -------- | ----- |
| `id`      | component ID | yes      | -     |
| `uart_id` | UART ID      | yes      | -     |

It exposes the following sensors:

| Key                | Type            | Required | Notes                                                                         |
| ------------------ | --------------- | -------- | ----------------------------------------------------------------------------- |
| `target_count`     | `sensor`        | no       | Number of targets in the current frame                                        |
| `vehicle_detected` | `binary_sensor` | no       | `ON` when device alarm fires (trigger_count met); `OFF` after no_target_delay |
| `angle`            | `sensor`        | no       | Degrees; negative = left of sensor axis                                       |
| `distance`         | `sensor`        | no       | Metres to nearest qualifying target                                           |
| `speed`            | `sensor`        | no       | km/h (after `speed_correction`)                                               |
| `snr`              | `sensor`        | no       | Signal-to-noise ratio (0..255)                                                |
| `direction`        | `text_sensor`   | no       | `Approaching`, `Moving away`, or `None`                                       |

UART validation is enforced for:

| Validation     | Requirement |
| -------------- | ----------- |
| UART baud rate | `115200`    |
| RX pin         | required    |

## Notes

| Topic              | Detail                                                                                                                                                                                                              |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Target publishing  | Current implementation publishes the nearest qualifying target per frame                                                                                                                                            |
| No-target behavior | After `no_target_delay`, target fields reset to `0`, direction resets to `None`, and `vehicle_detected` resets to `OFF`                                                                                             |
| Runtime settings   | Device-side parameters such as `max_distance`, `trigger_count`, and `min_snr` stay on the radar itself; ESPHome only consumes live frames.                                                                        |
| Distance filtering | `min_distance` is software-side only: targets closer than this value are suppressed, but a farther target from the same frame may still publish. `max_distance` is device-side only: the device enforces it, so ESPHome publishes whatever the device reports.                  |
| Speed correction   | `speed_correction` is software-side only: published speed is multiplied by this value                                                                                                                               |

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
| Parsed telemetry  | decoded target values (`targets`, `angle`, `distance`, `speed`, `direction`, `snr`) |
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
