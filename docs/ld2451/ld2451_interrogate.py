#!/usr/bin/env python3
"""
HLK-LD2451 mmWave Radar Interrogation Script
Protocol: V1.03 (Shenzhen Hailingke Electronics)
Baud: 115200, 8N1, little-endian

Usage:
    pip install pyserial
    python3 ld2451_interrogate.py
"""

import serial
from serial.tools import list_ports
import struct
import time
import sys
from datetime import datetime

PORT = None
BAUD = 115200

# Command frame delimiters
CMD_HEADER  = bytes([0xFD, 0xFC, 0xFB, 0xFA])
CMD_TAIL    = bytes([0x04, 0x03, 0x02, 0x01])

# Data output frame delimiters
DATA_HEADER = bytes([0xF4, 0xF3, 0xF2, 0xF1])
DATA_TAIL   = bytes([0xF8, 0xF7, 0xF6, 0xF5])


# ── Frame construction ────────────────────────────────────────────────────────

def build_cmd(cmd_word: int, cmd_value: bytes = b'') -> bytes:
    """Wrap a command word + value in the LD2451 command frame."""
    payload = struct.pack('<H', cmd_word) + cmd_value
    length  = struct.pack('<H', len(payload))
    return CMD_HEADER + length + payload + CMD_TAIL


# ── ACK reading ───────────────────────────────────────────────────────────────

def read_ack(ser: serial.Serial, timeout: float = 1.0):
    """
    Read one ACK frame from the device.
    Returns (success: bool, return_bytes: bytes) or (False, b'') on failure.
    """
    deadline = time.time() + timeout
    buf = b''

    # Scan for CMD_HEADER
    while time.time() < deadline:
        b = ser.read(1)
        if not b:
            continue
        buf += b
        if buf[-4:] == CMD_HEADER:
            buf = b''
            break
    else:
        return False, b''

    # Length (2 bytes)
    lb = ser.read(2)
    if len(lb) < 2:
        return False, b''
    length = struct.unpack('<H', lb)[0]

    # Payload + tail
    rest = ser.read(length + 4)
    if len(rest) < length + 4:
        return False, b''

    payload = rest[:length]
    tail    = rest[length:]

    if tail != CMD_TAIL:
        return False, b''

    if len(payload) < 4:
        return False, b''

    # ACK payload layout per observed LD2451 protocol responses:
    #   [0:2] command echo (request cmd + 0x0100)
    #   [2:4] status (0 = success)
    #   [4:]  return data
    status = struct.unpack('<H', payload[2:4])[0]
    return (status == 0), payload[4:]


# ── High-level commands ───────────────────────────────────────────────────────

def enable_config(ser) -> bool:
    """Must be called before any configuration command."""
    ser.write(build_cmd(0x00FF, bytes([0x00, 0x00])))
    ok, _ = read_ack(ser)
    return ok


def end_config(ser) -> bool:
    """Call after finishing configuration; resumes normal output."""
    ser.write(build_cmd(0x00FE))
    ok, _ = read_ack(ser)
    return ok


def read_firmware_version(ser):
    """
    Returns (fw_type, major, minor) or None.
    Example: (0x2451, 0x0101, 24051510) → V1.01.24051510
    """
    if not enable_config(ser):
        return None
    ser.write(build_cmd(0x00A1))
    ok, data = read_ack(ser)
    end_config(ser)
    if ok and len(data) >= 8:
        fw_type = struct.unpack('<H', data[0:2])[0]
        major   = struct.unpack('<H', data[2:4])[0]
        minor   = struct.unpack('<I', data[4:8])[0]
        return fw_type, major, minor
    return None


def read_detection_params(ser):
    """
    Returns (max_dist_m, direction, min_speed_kmh, no_target_delay_s) or None.
    direction: 0=away only, 1=approach only, 2=all
    """
    if not enable_config(ser):
        return None
    ser.write(build_cmd(0x0012))
    ok, data = read_ack(ser)
    end_config(ser)
    if ok and len(data) >= 4:
        return data[0], data[1], data[2], data[3]
    return None


def read_sensitivity_params(ser):
    """
    Returns raw 4-byte sensitivity config or None.
    Byte 0: cumulative trigger count (0 = default 1)
    Byte 1: SNR threshold level    (0 = default 4; range 3–8)
    """
    if not enable_config(ser):
        return None
    ser.write(build_cmd(0x0013))
    ok, data = read_ack(ser)
    end_config(ser)
    if ok and len(data) >= 4:
        return data[:4]
    return None


# ── Data frame parsing ────────────────────────────────────────────────────────

def parse_data_frame(frame_data: bytes):
    """
    Parse the intra-frame bytes from a radar output frame.
    Returns (target_count, approach_flag_active, targets_list) or None.

    Per-target dict: angle (°), distance (m), speed (km/h), direction, snr
    Direction byte: 0x01 = approaching, 0x00 = moving away
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
            'direction': 'Approaching' if dir_byte == 0x01 else 'Moving away',
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


# ── Pretty helpers ────────────────────────────────────────────────────────────

DIRECTION_LABELS = {0: 'Away only', 1: 'Approach only', 2: 'All directions'}


def fmt_direction(d):
    return DIRECTION_LABELS.get(d, f'Unknown (0x{d:02X})')


def fmt_fw(fw_type, major, minor):
    maj_hi = (major >> 8) & 0xFF
    maj_lo =  major       & 0xFF
    return f"Type=0x{fw_type:04X}  Version=V{maj_hi}.{maj_lo:02d}.{minor}"


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
    print("║   HLK-LD2451 Radar Interrogation ║")
    print("╚══════════════════════════════════╝\n")

    # ── Firmware ──────────────────────────────────────────────────────────────
    print("▶ Firmware version …")
    fw = read_firmware_version(ser)
    if fw:
        print(f"  {fmt_fw(*fw)}")
    else:
        print("  ✗ No response (device may already be streaming; continuing anyway)")

    # ── Detection parameters ──────────────────────────────────────────────────
    print("\n▶ Detection parameters …")
    params = read_detection_params(ser)
    if params:
        max_dist, direction, min_speed, delay = params
        print(f"  Max detection distance : {max_dist} m")
        print(f"  Detection direction    : {fmt_direction(direction)}")
        print(f"  Min detection speed    : {min_speed} km/h")
        print(f"  No-target delay        : {delay} s")
    else:
        print("  ✗ Could not read")

    # ── Sensitivity parameters ────────────────────────────────────────────────
    print("\n▶ Sensitivity parameters …")
    sens = read_sensitivity_params(ser)
    if sens:
        trigger_count = sens[0] if sens[0] != 0 else 1   # 0 means default=1
        snr_level     = sens[1] if sens[1] != 0 else 4   # 0 means default=4
        print(f"  Trigger count threshold : {trigger_count}  (consecutive hits before approach flag)")
        print(f"  SNR threshold level     : {snr_level}  (3–8; higher = less sensitive)")
    else:
        print("  ✗ Could not read")

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
