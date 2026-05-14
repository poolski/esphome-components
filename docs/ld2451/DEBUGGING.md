# LD2451 Debugging Runbook

This runbook helps verify whether the radar is sending bytes, whether frames are parseable, and whether ESPHome is decoding them.

## 1) Interrogate The Device Directly

Run from repo root:

```bash
python3 docs/ld2451/ld2451_interrogate.py
```

What this confirms:

- serial device discovery works (USB/UART bridge found)
- configuration commands can be ACKed
- live frames are parseable in Python

If no bridge is found, reconnect the USB/UART device and re-run.

## 2) Run Parser/Discovery Unit Tests

```bash
python3 -m unittest -v docs/ld2451/test_ld2451_parse.py
```

This verifies:

- frame parsing behavior
- direction mapping
- USB/UART bridge discovery logic

## 3) Build-Validate The ESPHome Component

```bash
./scripts/test-component.sh components/ld2451
```

This ensures config/compile still pass after debugging changes.

## 4) Enable ESPHome Logging For Runtime Checks

Use at least:

```yaml
logger:
  level: DEBUG
```

The component now emits periodic receive diagnostics:

- `RX activity: read=<n> bytes, buffered=<m> bytes`
- `Frame <n> has short payload len=<x> (short frames=<y>)`

These logs confirm UART activity even when payloads are short/empty.

## 5) Troubleshooting Checklist

- **No serial port found**: USB/UART adapter disconnected or different device node.
- **RX activity present, no parsed detections**: radar may be emitting empty/short payload frames while idle.
- **Interrogator parses frames but ESPHome does not**: compare frame bytes and firmware behavior under the same wiring and baud settings.
- **No ACK for config commands**: radar may already be streaming/busy, or command timing/state is incorrect.
