from dataclasses import dataclass
from typing import Self
from py_core.net.node.cluster_id import ClusterId
from py_core.net.node.destination import Destination

from py_core.net.node.node_id import NodeId
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize


@dataclass(frozen=True)
class Source(DeriveDeserialize, DeriveSerialize):
    node_id: NodeId
    cluster_id: ClusterId

    @classmethod
    def broadcast(cls) -> Self:
        return cls(NodeId.broadcast(), ClusterId.no_cluster())

    @classmethod
    def loopback(cls) -> Self:
        return cls(NodeId.loopback(), ClusterId.no_cluster())

    def into_destination(self) -> Destination:
        return Destination(self.node_id, self.cluster_id)

    @classmethod
    def from_destination(cls, destination: Destination) -> Self:
        return cls(destination.node_id, destination.cluster_id)

    def matches(self, destination: Destination) -> bool:
        if destination.is_broadcast():
            return True
        if destination.is_unicast():
            return self.node_id == destination.node_id
        return self.cluster_id == destination.cluster_id
