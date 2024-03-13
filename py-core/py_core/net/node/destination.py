from dataclasses import dataclass
from typing import Self
from py_core.net.node.cluster_id import ClusterId

from py_core.net.node.node_id import NodeId
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize


@dataclass(frozen=True)
class Destination(DeriveDeserialize, DeriveSerialize):
    node_id: NodeId
    cluster_id: ClusterId

    @classmethod
    def broadcast(cls) -> Self:
        return cls(NodeId.broadcast(), ClusterId.no_cluster())

    def is_broadcast(self) -> bool:
        return self.node_id.is_broadcast() and self.cluster_id.is_no_cluster()

    def is_multicast(self) -> bool:
        return self.node_id.is_broadcast() and not self.cluster_id.is_no_cluster()

    def is_unicast(self) -> bool:
        return not self.node_id.is_broadcast()

    @classmethod
    def loopback(cls) -> Self:
        return cls(NodeId.loopback(), ClusterId.no_cluster())

    def is_loopback(self) -> bool:
        return self.node_id.is_loopback()
