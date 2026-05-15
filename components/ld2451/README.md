# LD2451 ESPHome External Component

ESPHome external component for the HLK-LD2451 radar over UART (115200 baud).

This component parses LD2451 live data frames and exposes key values as ESPHome entities.

## Features

- Parses LD2451 data frames (`F4 F3 F2 F1 ... F8 F7 F6 F5`)
- Publishes:
  - target count
  - alarm flag
  - first target angle
  - first target distance
  - first target speed
  - first target SNR
  - first target direction
- Direction mapping follows verified LD2451 stream behavior:
  - `0x01` => `approaching`
  - `0x00` => `moving_away`

## Installation

Add this component from GitHub in your ESPHome YAML.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/poolski/esphome-components
      ref: main
    path: components
    refresh: 1d
    components: [ld2451]
```

`refresh` sets how often ESPHome refreshes the GitHub source cache. `1d` is a good default.
Use a shorter value while developing and a longer value if you want fewer update checks.

Equivalent shorthand:

```yaml
external_components:
  - source: github://poolski/esphome-components@main
    path: components
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
      ref: main
    path: components
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
  alarm:
    name: "LD2451 Alarm"
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

- `id` (required)
- `uart_id` (required)
- `max_distance` (optional): `10..100` (radar-side + software upper cap, default `100`)
- `min_distance` (optional): `0..100` (software publish filter, default `0`)
- `min_speed` (optional): `0..120` (radar-side)
- `detection_direction` (optional): `away` / `approach` / `both` (radar-side)
- `no_target_delay` (optional): `0..255` seconds (radar-side)
- `trigger_count` (optional): `1..10` (radar-side sensitivity)
- `min_snr` (optional): `0` or `3..8` (radar-side sensitivity)
- `speed_correction` (optional): `0.1..4.0` multiplier for published speed (default `1.0`)
- `target_count` (optional `sensor`)
- `alarm` (optional `binary_sensor`)
- `angle` (optional `sensor`)
- `distance` (optional `sensor`)
- `speed` (optional `sensor`)
- `snr` (optional `sensor`)
- `direction` (optional `text_sensor`)
- `controls` (optional runtime entities):
  - numeric controls: `max_distance`, `min_distance`, `min_speed`, `no_target_delay`, `trigger_count`, `min_snr`, `app_snr_threshold`, `speed_correction`
  - select control: `detection_direction` (`away`, `approach`, `both`)
  - runtime note: `min_snr` only accepts `0` or `3..8`; runtime values `1` and `2` are coerced to `0`
  - runtime note: `app_snr_threshold` accepts `0..64` and is mapped to the native LD2451 `min_snr` scale (`0`, `3..8`)

UART validation is enforced for:

- baud rate `115200`
- RX pin required

## Notes

- Current implementation publishes only the first target details (`target 1`) per frame.
- If no target is present, only `target_count` and `alarm` are updated.
- `min_distance` is software-side only: targets outside `min_distance..max_distance` are filtered from published target fields.
- `speed_correction` is software-side only: published speed is multiplied by this value.

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
    app_snr_threshold:
      name: "LD2451 App SNR Threshold"
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

- Parsed target frames with decoded values (`targets`, `alarm`, `angle`, `distance`, `speed`, `direction`, `snr`)
- Periodic hints when only empty frames are received

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
