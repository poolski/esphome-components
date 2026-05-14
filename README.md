# ESPHome Components

Custom ESPHome external components maintained in this repository.

## Components

- `ld2451` - HLK-LD2451 UART radar component

## Install and Use

Reference this repository from your ESPHome config:

```yaml
external_components:
  - source: github://poolski/esphome-components@main
    components: [ld2451]
```

Then add the component block (minimal example):

```yaml
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

## `ld2451` README Summary

`ld2451/README.md` covers:

- parsed LD2451 frame format and exposed entities
- verified direction mapping (`0x01` approaching, `0x00` moving away)
- complete YAML example, including debug logging
- troubleshooting guidance when no detections appear
- movement guidance for bench testing (larger toward-sensor motion may be required)
- per-component local validation command

## Development and Testing

This repository is designed for multiple components. CI and local checks run per component directory.

Run a component test locally:

```bash
./scripts/test-component.sh ld2451
```
