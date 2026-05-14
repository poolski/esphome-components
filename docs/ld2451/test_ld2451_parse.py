import importlib.util
import pathlib
import unittest
from unittest.mock import patch


def load_module():
    path = pathlib.Path(__file__).with_name("ld2451_interrogate.py")
    spec = importlib.util.spec_from_file_location("ld2451_module", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class ParseDataFrameTests(unittest.TestCase):
    def test_parse_includes_raw_target_bytes(self):
        m = load_module()
        frame = bytes([1, 1, 0x80, 2, 1, 3, 136])

        result = m.parse_data_frame(frame)

        self.assertIsNotNone(result)
        target_count, alarm, targets = result
        self.assertEqual(target_count, 1)
        self.assertTrue(alarm)
        self.assertEqual(targets[0]["raw_angle"], 0x80)
        self.assertEqual(targets[0]["raw_distance"], 2)
        self.assertEqual(targets[0]["raw_direction"], 1)
        self.assertEqual(targets[0]["raw_speed"], 3)
        self.assertEqual(targets[0]["raw_snr"], 136)

    def test_direction_byte_one_maps_to_approaching(self):
        m = load_module()
        frame = bytes([1, 1, 0x80, 2, 1, 3, 136])

        result = m.parse_data_frame(frame)

        self.assertIsNotNone(result)
        _, _, targets = result
        self.assertEqual(targets[0]["direction"], "approaching")


class PortDiscoveryTests(unittest.TestCase):
    class _FakePort:
        def __init__(self, device, description="", manufacturer="", product="", interface=""):
            self.device = device
            self.description = description
            self.manufacturer = manufacturer
            self.product = product
            self.interface = interface

    def test_discovers_usb_uart_bridge_port(self):
        m = load_module()
        ports = [
            self._FakePort("/dev/cu.Bluetooth-Incoming-Port", description="Bluetooth"),
            self._FakePort("/dev/cu.usbmodem123", description="USB UART Bridge"),
        ]

        with patch("serial.tools.list_ports.comports", return_value=ports):
            port = m.discover_uart_port()

        self.assertEqual(port, "/dev/cu.usbmodem123")

    def test_uses_preferred_port_if_present(self):
        m = load_module()
        ports = [
            self._FakePort("/dev/cu.usbserial-1", description="USB Serial"),
            self._FakePort("/dev/cu.usbmodemflip_Zer01", description="Flipper Zero"),
        ]

        with patch("serial.tools.list_ports.comports", return_value=ports):
            port = m.discover_uart_port(preferred="/dev/cu.usbmodemflip_Zer01")

        self.assertEqual(port, "/dev/cu.usbmodemflip_Zer01")

    def test_raises_if_no_bridge_ports_found(self):
        m = load_module()
        ports = [
            self._FakePort("/dev/cu.Bluetooth-Incoming-Port", description="Bluetooth"),
        ]

        with patch("serial.tools.list_ports.comports", return_value=ports):
            with self.assertRaises(RuntimeError):
                m.discover_uart_port()


if __name__ == "__main__":
    unittest.main()
