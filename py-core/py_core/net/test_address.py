from ipaddress import IPv4Address
import unittest

from .address import (
    SerialAddress,
    UdpAddress,
    UhfAddress,
    WebSocketAddress,
)


class TestAddress(unittest.TestCase):
    def check_equality(self, a, b):
        self.assertEqual(a, b)
        self.assertEqual(hash(a), hash(b))

    def test_serial_address_equality(self):
        a1 = SerialAddress(0x01)
        a2 = SerialAddress(0x01)
        self.check_equality(a1, a2)

    def test_uhf_address_equality(self):
        a1 = UhfAddress(0x01)
        a2 = UhfAddress(0x01)
        self.check_equality(a1, a2)

    def test_udp_address_equality(self):
        a1 = UdpAddress(IPv4Address("192.0.2.1"), 12345)
        a2 = UdpAddress(IPv4Address("192.0.2.1"), 12345)
        self.check_equality(a1, a2)

    def test_websocket_address_equality(self):
        a1 = WebSocketAddress(IPv4Address("192.0.2.1"), 12345)
        a2 = WebSocketAddress(IPv4Address("192.0.2.1"), 12345)
        self.check_equality(a1, a2)
