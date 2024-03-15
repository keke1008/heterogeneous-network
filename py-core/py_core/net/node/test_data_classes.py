import unittest

from py_core.net.link.address import Address, UhfAddress
from py_core.net.node.destination import Destination
from py_core.net.node.source import Source
from py_core.serde.primitives import UInt8

from .node_id import NodeId
from .cluster_id import ClusterId


class TestDataClasses(unittest.TestCase):
    def check_equality[T](self, a: T, b: T):
        self.assertEqual(a, b)
        self.assertEqual(hash(a), hash(b))

    def test_node_id_equality(self):
        node_id_1 = NodeId.from_address(Address(UhfAddress(0x01)))
        node_id_2 = NodeId.from_address(Address(UhfAddress(0x01)))
        self.check_equality(node_id_1, node_id_2)

    def test_cluster_id_equality(self):
        cluster_id_1 = ClusterId(UInt8(0x01))
        cluster_id_2 = ClusterId(UInt8(0x01))
        self.check_equality(cluster_id_1, cluster_id_2)

    def test_source_equality(self):
        source_1 = Source(
            NodeId.from_address(Address(UhfAddress(0x01))), ClusterId(UInt8(0x01))
        )
        source_2 = Source(
            NodeId.from_address(Address(UhfAddress(0x01))), ClusterId(UInt8(0x01))
        )
        self.check_equality(source_1, source_2)

    def test_destination_equality(self):
        d1 = Destination(
            NodeId.from_address(Address(UhfAddress(0x01))), ClusterId(UInt8(0x01))
        )
        d2 = Destination(
            NodeId.from_address(Address(UhfAddress(0x01))), ClusterId(UInt8(0x01))
        )
        self.check_equality(d1, d2)

    def test_cost_equality(self):
        cost_1 = UInt8(0x01)
        cost_2 = UInt8(0x01)
        self.check_equality(cost_1, cost_2)
