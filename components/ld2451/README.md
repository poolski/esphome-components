# LD2451 ESPHome External Component

ESPHome external component for the HLK-LD2451 radar over UART (115200 baud).

This component parses LD2451 live data frames and exposes key values as ESPHome entities.

## Features

| Capability        | Details                                                                                                                           |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| Frame parsing     | Parses LD2451 data frames (`F4 F3 F2 F1 ... F8 F7 F6 F5`)                                                                         |
| Published data    | target count, vehicle detected, first target angle, first target distance, first target speed, first target SNR, first target direction |
| Direction mapping | `0x01` => `Approaching`, `0x00` => `Moving away`, idle => `None`                                                                  |

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

| Key                   | Type            | Required | Range/Options                | Notes                                          |
| --------------------- | --------------- | -------- | ---------------------------- | ---------------------------------------------- |
| `id`                  | component ID    | yes      | -                            | -                                              |
| `uart_id`             | UART ID         | yes      | -                            | -                                              |
| `max_distance`        | integer         | no       | `10..100`                    | radar-side + software upper cap, default `100` |
| `min_distance`        | integer         | no       | `0..100`                     | software publish filter, default `0`           |
| `min_speed`           | integer         | no       | `0..120`                     | radar-side                                     |
| `detection_direction` | enum            | no       | `away` / `approach` / `both` | radar-side                                     |
| `no_target_delay`     | integer         | no       | `0..255` seconds             | radar-side                                     |
| `trigger_count`       | integer         | no       | `1..10`                      | radar-side sensitivity                         |
| `min_snr`             | integer         | no       | `0` or `3..8`                | radar-side sensitivity                         |
| `speed_correction`    | float           | no       | `0.1..4.0`                   | multiplier for published speed, default `1.0`  |
| `target_count`        | `sensor`        | no       | -                            | -                                              |
| `vehicle_detected`    | `binary_sensor` | no       | -                            | trigger-friendly detection state               |
| `angle`               | `sensor`        | no       | -                            | -                                              |
| `distance`            | `sensor`        | no       | -                            | -                                              |
| `speed`               | `sensor`        | no       | -                            | -                                              |
| `snr`                 | `sensor`        | no       | -                            | -                                              |
| `direction`           | `text_sensor`   | no       | -                            | -                                              |
| `controls`            | object          | no       | see below                    | runtime entities                               |

`controls` details:

| Control group    | Details                                                                                                                         |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| Numeric controls | `max_distance`, `min_distance`, `min_speed`, `no_target_delay`, `trigger_count`, `min_snr`, `snr_threshold`, `speed_correction` |
| Select controls  | `detection_direction` (`away`, `approach`, `both`)                                                                              |
| Runtime notes    | `min_snr` only accepts `0` or `3..8`; runtime values `1` and `2` are coerced to `0`                                             |
| Runtime notes    | `snr_threshold` accepts `0..64` and is mapped to the native LD2451 `min_snr` scale (`0`, `3..8`)                                |

## Runtime Config Sync

- Device is source of truth.
- Component reads at boot.
- Component polls every 5 seconds.
- Post-write readback publishes actual values.
- Bluetooth-side changes reflected within 5 seconds.

UART validation is enforced for:

| Validation     | Requirement |
| -------------- | ----------- |
| UART baud rate | `115200`    |
| RX pin         | required    |

## Notes

| Topic              | Detail                                                                                                                       |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------- |
| Target publishing  | Current implementation publishes only the first target details (`target 1`) per frame                                        |
| No-target behavior | After `no_target_delay`, target fields reset to `0`, direction resets to `None`, and `vehicle_detected` resets to `OFF`     |
| Distance filtering | `min_distance` is software-side only: targets outside `min_distance..max_distance` are filtered from published target fields |
| Speed correction   | `speed_correction` is software-side only: published speed is multiplied by this value                                        |

### Home Assistant automation trigger

Use `vehicle_detected` (`off` -> `on`) as the trigger in HA automations. The speed, distance, and direction sensors are updated on qualifying detections, and reset when idle.

## Runtime Controls Example

```yaml
ld2451:
  id: radar
  uart_id: uart_bus
  max_distance: 100
  min_distance: 5
  min_speed: 10
  detection_direction: both
  no_target_delay: 2
  trigger_count: 2
  min_snr: 4
  speed_correction: 1.05
  controls:
    max_distance:
      name: "LD2451 Max Distance"
    min_distance:
      name: "LD2451 Min Distance"
    min_speed:
      name: "LD2451 Min Speed"
    no_target_delay:
      name: "LD2451 No Target Delay"
    trigger_count:
      name: "LD2451 Trigger Count"
    min_snr:
      name: "LD2451 Min SNR"
    snr_threshold:
      name: "LD2451 SNR Threshold"
    speed_correction:
      name: "LD2451 Speed Correction"
    detection_direction:
      name: "LD2451 Detection Direction"
```

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

| Log type          | Details                                                                               |
| ----------------- | ------------------------------------------------------------------------------------- |
| Parsed frames     | decoded values (`targets`, `angle`, `distance`, `speed`, `direction`, `snr`)          |
| Empty-frame hints | periodic hints when only empty frames are received                                    |

Important for pre-install bench checks: you may need large, deliberate movement to trigger detection
(for example, walking or running toward the sensor, or broad hand/body motion). Small or static motion often does not
trigger frames, especially with restrictive sensor-side settings.

## Troubleshooting: No Detections

1. Confirm UART and component setup
   - `baud_rate: 115200`
   - correct `rx_pin` wiring from sensor TX
   - `external_components` uses `source: github://poolski/esphome-components@main`
2. Turn on debug logs and watch runtime frames
   - set `logger.level: DEBUG`
   - look for LD2451 debug lines (parsed frames and empty-frame hints)
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
