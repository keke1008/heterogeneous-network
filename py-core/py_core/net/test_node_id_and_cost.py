import unittest

from py_core.serde import UInt8

from .address import Address, UhfAddress
from .node_id import NodeId


class TestDataClasses(unittest.TestCase):
    def check_equality[T](self, a: T, b: T):
        self.assertEqual(a, b)
        self.assertEqual(hash(a), hash(b))

    def test_node_id_equality(self):
        node_id_1 = NodeId.from_address(Address(UhfAddress(0x01)))
        node_id_2 = NodeId.from_address(Address(UhfAddress(0x01)))
        self.check_equality(node_id_1, node_id_2)

    def test_cost_equality(self):
        cost_1 = UInt8(0x01)
        cost_2 = UInt8(0x01)
        self.check_equality(cost_1, cost_2)
