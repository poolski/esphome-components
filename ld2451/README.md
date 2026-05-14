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
  - source: github://poolski/esphome-components@main
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
  - source: github://poolski/esphome-components@main
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
- `target_count` (optional `sensor`)
- `alarm` (optional `binary_sensor`)
- `angle` (optional `sensor`)
- `distance` (optional `sensor`)
- `speed` (optional `sensor`)
- `snr` (optional `sensor`)
- `direction` (optional `text_sensor`)
- `bluetooth_enabled` (optional `switch`) runtime BLE control (`ON` enables BLE, `OFF` disables BLE)
- `disable_bluetooth_on_boot` (optional `bool`, default `false`) sends BLE-disable command during setup

UART validation is enforced for:

- baud rate `115200`
- RX pin required

### Bluetooth Runtime Control

You can toggle module Bluetooth at runtime and optionally disable it on boot.

```yaml
ld2451:
  id: radar
  uart_id: uart_bus
  disable_bluetooth_on_boot: true
  bluetooth_enabled:
    name: "LD2451 Bluetooth Enabled"
```

Behavior:

- switch `OFF`: sends command `0xA4` with payload `00 00`
- switch `ON`: sends command `0xA4` with payload `01 00`
- command sequence is wrapped by config enter/exit commands (`0xFF` / `0xFE`)

Note: command `0xA4` is field-validated but not listed in published LD2451 serial protocol command tables;
behavior may vary by module firmware.

## Notes

- Current implementation publishes only the first target details (`target 1`) per frame.
- If no target is present, only `target_count` and `alarm` are updated.

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
../scripts/test-component.sh ld2451
```
