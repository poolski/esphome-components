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

| Key                | Type            | Required | Notes                                    |
| ------------------ | --------------- | -------- | ---------------------------------------- |
| `id`               | component ID    | yes      | -                                        |
| `uart_id`          | UART ID         | yes      | -                                        |
| `target_count`     | `sensor`        | no       | Number of targets in the current frame   |
| `vehicle_detected` | `binary_sensor` | no       | `ON` when device alarm fires (trigger_count met); `OFF` after no_target_delay |
| `angle`            | `sensor`        | no       | Degrees; negative = left of sensor axis  |
| `distance`         | `sensor`        | no       | Metres to closest qualifying target      |
| `speed`            | `sensor`        | no       | km/h (after `speed_correction`)          |
| `snr`              | `sensor`        | no       | Signal-to-noise ratio (0..255)           |
| `direction`        | `text_sensor`   | no       | `Approaching`, `Moving away`, or `None`  |
| `controls`         | object          | no       | Runtime-configurable entities (see below)|

### `controls` parameters

All device-stored parameters are written to flash and survive power cycles.

| Control               | Range / Options              | Default | Where stored  | Description (from HLK-LD2451 User Manual §5.2)                                                                            |
| --------------------- | ---------------------------- | ------- | ------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `max_distance`        | `10..100` m                  | `100`   | Device        | Farthest detection distance. Targets beyond this are ignored by the device.                                               |
| `min_distance`        | `0..100` m                   | `0`     | Software only | Publish filter applied in ESPHome. Targets closer than this are not reported to Home Assistant.                           |
| `min_speed`           | `0..120` km/h                | `0`     | Device        | Minimum speed a target must exceed to be reported. Slower targets are ignored.                                            |
| `detection_direction` | `away` / `approach` / `both` | `both`  | Device        | `approach`: same-direction vehicles only. `away`: opposite-direction vehicles only. `both`: all directions.               |
| `no_target_delay`     | `0..255` s                   | `0`     | Device        | How long after the last detection the device continues reporting the target. Resets if a new detection occurs within this window. |
| `trigger_count`       | `1..10`                      | `1`     | Device        | Consecutive detections required before the device sets its alarm flag. Only `vehicle_detected` is gated on this; sensor data (distance, speed, etc.) is published as soon as any qualifying target appears. |
| `min_snr`             | `0` or `3..8`                | `0`     | Device        | `0` = device default (equivalent to 4). `3..8`: lower = more sensitive, easier to trigger; higher = less sensitive, harder. Not recommended to change unless needed. |
| `snr_threshold`       | `0..64`                      | `0`     | Device        | Alternative app-scale SNR input (0..64) that maps to native `min_snr` levels (`0`, `3..8`). Use instead of `min_snr`.    |
| `speed_correction`    | `0.1..4.0`                   | `1.0`   | Software only | Multiplier applied to the published speed value. Does not affect the device.                                              |

## Runtime Config Sync

- Device is source of truth on boot.
- Component reads device config once at startup and populates the `controls` entities.
- Post-write readback confirms actual device values after any change.
- Use the `controls` entities in Home Assistant to adjust parameters at runtime.

UART validation is enforced for:

| Validation     | Requirement |
| -------------- | ----------- |
| UART baud rate | `115200`    |
| RX pin         | required    |

## Notes

| Topic              | Detail                                                                                                                       |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------- |
| Target publishing  | Current implementation publishes only the first target details (`target 1`) per frame                                        |
| No-target behavior | After `no_target_delay`, target fields reset to `0`, direction resets to `None`, and `vehicle_detected` resets to `OFF`      |
| Trigger count      | `vehicle_detected` is gated on the device alarm flag (set only after `trigger_count` consecutive detections). Distance, speed, angle, and SNR are published for any qualifying target, even before the alarm fires. |
| Distance filtering | `min_distance` is software-side only: targets outside `min_distance..max_distance` are filtered from published target fields |
| Speed correction   | `speed_correction` is software-side only: published speed is multiplied by this value                                        |

### Home Assistant automation trigger

Use `vehicle_detected` (`off` -> `on`) as the trigger in HA automations. The speed, distance, and direction sensors are updated on qualifying detections, and reset when idle.

## Runtime Controls Example

Configure all parameters from Home Assistant via `controls:` entities. The device is the source of
truth on boot; values set here persist to device flash and survive power cycles.

```yaml
ld2451:
  id: radar
  uart_id: uart_bus
  controls:
    max_distance:
      name: "LD2451 Max Distance"      # 10..100 m, device-stored
    min_distance:
      name: "LD2451 Min Distance"      # 0..100 m, software filter only
    min_speed:
      name: "LD2451 Min Speed"         # 0..120 km/h, device-stored
    no_target_delay:
      name: "LD2451 No Target Delay"   # 0..255 s, device-stored
    trigger_count:
      name: "LD2451 Trigger Count"     # 1..10, device-stored
    snr_threshold:
      name: "LD2451 SNR Threshold"     # 0..64 app scale → native 0/3..8, device-stored
    speed_correction:
      name: "LD2451 Speed Correction"  # 0.1..4.0 multiplier, software only
    detection_direction:
      name: "LD2451 Detection Direction"  # away / approach / both, device-stored
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
