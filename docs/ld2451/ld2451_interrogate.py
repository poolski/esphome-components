#!/usr/bin/env python3
"""
HLK-LD2451 mmWave Radar Live Frame Inspector
Protocol: V1.03 (Shenzhen Hailingke Electronics)
Baud: 115200, 8N1, little-endian

Usage:
    pip install pyserial
    python3 ld2451_interrogate.py
"""

import serial
from serial.tools import list_ports
import time
import sys
from datetime import datetime

PORT = None
BAUD = 115200

# Data output frame delimiters
DATA_HEADER = bytes([0xF4, 0xF3, 0xF2, 0xF1])
DATA_TAIL   = bytes([0xF8, 0xF7, 0xF6, 0xF5])


# ── Data frame parsing ────────────────────────────────────────────────────────

def parse_data_frame(frame_data: bytes):
    """
    Parse the intra-frame bytes from a radar output frame.
    Returns (target_count, approach_flag_active, targets_list) or None.

    Per-target dict: angle (°), distance (m), speed (km/h), direction, snr
    Direction byte: 0x00 = approaching, 0x01 = moving away
    """
    if len(frame_data) < 2:
        return None

    target_count = frame_data[0]
    approach_flag = bool(frame_data[1])  # Protocol approach flag (1 = approaching target present)
    targets      = []
    offset       = 2

    for _ in range(target_count):
        if offset + 5 > len(frame_data):
            break
        angle_raw   = frame_data[offset]
        distance    = frame_data[offset + 1]
        dir_byte    = frame_data[offset + 2]   # 0=approaching, 1=away
        speed       = frame_data[offset + 3]
        snr         = frame_data[offset + 4]

        targets.append({
            'angle':     angle_raw - 0x80,          # signed degrees
            'distance':  distance,                   # metres
            'direction': 'Approaching' if dir_byte == 0x00 else 'Moving away',
            'speed':     speed,                      # km/h
            'snr':       snr,
            'raw_angle': angle_raw,
            'raw_distance': distance,
            'raw_direction': dir_byte,
            'raw_speed': speed,
            'raw_snr': snr,
        })
        offset += 5

    return target_count, approach_flag, targets


def stream_data_frames(ser, count: int = 20, total_timeout: float = 15.0):
    """
    Generator: yields parsed frames until `count` frames received or timeout.
    """
    ser.timeout = 0.05
    buf = b''
    frames = 0
    deadline = time.time() + total_timeout

    while frames < count and time.time() < deadline:
        buf += ser.read(512)

        idx = buf.find(DATA_HEADER)
        if idx == -1:
            buf = buf[-3:]          # keep tail to catch split headers
            continue
        buf = buf[idx:]             # trim to first header

        if len(buf) < 6:
            continue                # wait for length bytes

        length = struct.unpack('<H', buf[4:6])[0]
        total  = 4 + 2 + length + 4   # header + len_field + data + tail

        if len(buf) < total:
            continue                # wait for full frame

        frame = buf[:total]
        if frame[-4:] != DATA_TAIL:
            buf = buf[1:]           # bad frame sync, skip one byte
            continue

        result = parse_data_frame(frame[6:6 + length])
        if result:
            frames += 1
            yield result

        buf = buf[total:]


def discover_uart_port(preferred: str | None = None) -> str:
    """
    Discover a likely USB/UART serial bridge device.
    If `preferred` is provided and present, use it.
    """
    ports = list(list_ports.comports())
    if preferred:
        for p in ports:
            if p.device == preferred:
                return p.device

    def text(p):
        return " ".join([
            (p.description or ""),
            (p.manufacturer or ""),
            (p.product or ""),
            (getattr(p, "interface", "") or ""),
        ]).lower()

    positive_keywords = (
        "usb", "uart", "serial", "cdc", "cp210", "ch340", "ftdi", "silicon labs", "bridge", "flipper"
    )
    negative_keywords = ("bluetooth", "debug-console")

    candidates = []
    for p in ports:
        t = text(p)
        if any(n in t for n in negative_keywords):
            continue
        if any(k in t for k in positive_keywords):
            candidates.append(p.device)

    if not candidates:
        raise RuntimeError("No USB/UART bridge serial ports found")

    for dev in candidates:
        if dev.startswith("/dev/cu."):
            return dev
    return candidates[0]


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    preferred = PORT
    try:
        port = discover_uart_port(preferred=preferred)
    except RuntimeError as e:
        print(f"\nERROR: {e}")
        print("Connect a USB-UART bridge and try again.")
        sys.exit(1)

    print(f"Connecting to {port} @ {BAUD} baud …")
    try:
        ser = serial.Serial(port, BAUD, timeout=1.0)
    except serial.SerialException as e:
        print(f"\nERROR: {e}")
        print("Is a USB-UART bridge connected and available?")
        sys.exit(1)

    time.sleep(0.3)
    ser.reset_input_buffer()

    print("\n╔══════════════════════════════════╗")
    print("║   HLK-LD2451 Live Frame Inspector ║")
    print("╚══════════════════════════════════╝\n")

    # ── Live data stream ──────────────────────────────────────────────────────
    print("\n▶ Live data — capturing 20 frames (15 s timeout) …\n")
    ser.reset_input_buffer()

    try:
        for i, (count, approach_flag, targets) in enumerate(stream_data_frames(ser, count=20)):
            ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            approach_str = " ⚠  APPROACH FLAG" if approach_flag else ""
            print(f"[{ts}] frame {i+1:02d} | targets: {count}{approach_str}")
            if targets:
                for j, t in enumerate(targets):
                    print(f"         target {j+1}: "
                          f"angle={t['angle']:+4d}°  "
                          f"dist={t['distance']:3d} m  "
                          f"speed={t['speed']:3d} km/h  "
                          f"({t['direction']})  "
                          f"SNR={t['snr']}  "
                          f"raw=[{t['raw_angle']:02X} {t['raw_distance']:02X} "
                          f"{t['raw_direction']:02X} {t['raw_speed']:02X} {t['raw_snr']:02X}]")
            else:
                print("         (no targets detected)")
    except KeyboardInterrupt:
        print("\nInterrupted by user.")

    ser.close()
    print("\nDone — port closed.")


if __name__ == '__main__':
    main()
