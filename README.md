# ESPHome Components

Custom ESPHome external components maintained in this repository.

## Components

- [`ld2451`](components/ld2451/README.md) - HLK-LD2451 UART radar component

## Install and Use

Reference this repository from your ESPHome config:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/poolski/esphome-components
    refresh: 1d
```

`refresh` controls how often ESPHome checks the remote source for updates.
Use shorter values while actively iterating, and longer values for stable deployments.

Equivalent shorthand:

```yaml
external_components:
  - source: github://poolski/esphome-components
    refresh: 1d
```

By default, ESPHome loads all components from that source. You can optionally limit to a subset:

```yaml
external_components:
  - source: github://poolski/esphome-components
    refresh: 1d
    components: [ld2451]
```

Then configure whichever component(s) you want to use. Each component has its own README with full YAML examples.

## LD2451

See [`components/ld2451/README.md`](components/ld2451/README.md) for setup, configuration, debugging, and troubleshooting details.

| Area                           | Details                                                         |
| ------------------------------ | --------------------------------------------------------------- |
| UART parsing                   | HLK-LD2451 live frame parsing over UART (115200)                |
| Exposed entities               | target count, vehicle_detected, angle, distance, speed, SNR, and direction |
| Diagnostics                    | debug logging guidance and movement-based bench test advice     |
| Validation and troubleshooting | per-component validation command and troubleshooting notes      |

## Development and Testing

This repository is designed for multiple components. CI and local checks run per component directory.

Run a component test locally:

```bash
./scripts/test-component.sh components/ld2451
```
