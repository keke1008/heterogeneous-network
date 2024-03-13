import asyncio
from dataclasses import dataclass
from enum import Flag

from py_core.net.link.address import Address
from py_core.net.link.frame import Protocol, ProtocolValue
from py_core.net.link.service import LinkService, LinkSocket
from py_core.net.local.service import LocalNodeService
from py_core.net.neighbor.table import (
    NeighborNode,
    NeighborTable,
    NeighborTableListener,
)
from py_core.net.node import NodeId, Cost
from py_core.net.notification.service import NeighborRemoved, NeighborUpdated
from py_core.serde.buffer import BufferWriter
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize
from py_core.serde.enum import EnumSerde
from py_core.net.notification import NotificationService


class NeighborControlFlagValue(Flag):
    Empty = 0
    KeepAlive = 1


@dataclass(frozen=True)
class NeighborControlFlag(EnumSerde[NeighborControlFlagValue]):
    @staticmethod
    def enum_type():
        return NeighborControlFlagValue


@dataclass(frozen=True)
class NeighborControlFrame(DeriveDeserialize, DeriveSerialize):
    flags: NeighborControlFlag
    source_node_id: NodeId
    link_cost: Cost

    def should_reply_immediately(self) -> bool:
        return (self.flags.value & NeighborControlFlagValue.KeepAlive) == 0


class NeighborService:
    _notification_service: NotificationService
    _local_node_service: LocalNodeService

    _neighbor_table: NeighborTable
    _socket: LinkSocket

    def __init__(
        self,
        link_service: LinkService,
        notification_service: NotificationService,
        local_node_service: LocalNodeService,
    ):
        self._notification_service = notification_service
        self._local_node_service = local_node_service

        self._neighbor_table = NeighborTable()
        self._socket = link_service.open(Protocol(ProtocolValue.RoutingNeighbor))

        this = self

        class Listener(NeighborTableListener):
            def on_neighbor_updated(self, neighbor: NeighborNode) -> None:
                notification = NeighborUpdated(neighbor.node_id, neighbor.link_cost)
                notification_service.notify(notification)

            def on_neighbor_removed(self, neighbor: NeighborNode) -> None:
                notification = NeighborRemoved(neighbor.node_id)
                notification_service.notify(notification)

            def on_neighbor_should_send_hello(self, neighbor: NeighborNode) -> None:
                address = next(iter(neighbor.addresses))
                flags = NeighborControlFlag(NeighborControlFlagValue.Empty)
                asyncio.create_task(
                    this._send_hello(address, neighbor.link_cost, flags)
                )

        self._neighbor_table.add_listener(Listener())

    async def _send_hello(
        self, address: Address, link_cost: Cost, flags: NeighborControlFlag
    ) -> None:
        frame = NeighborControlFrame(
            flags=flags,
            source_node_id=await self._local_node_service.node_id(),
            link_cost=link_cost,
        )

        writer = BufferWriter(frame.serialized_length())
        frame.serialize(writer)
        await self._socket.send(address, writer.unwrap_buffer())

    async def send_hello(self, address: Address, link_cost: Cost) -> None:
        await self._send_hello(
            address, link_cost, NeighborControlFlag(NeighborControlFlagValue.Empty)
        )

    def get_neighbor(self, node_id: NodeId) -> NeighborNode | None:
        return self._neighbor_table.get_neighbor(node_id)

    def resolve_neighbor_from_address(self, address: Address) -> NeighborNode | None:
        return self._neighbor_table.resolve_neighbor_from_address(address)

    def on_frame_sent(self, destination: NodeId) -> None:
        self._neighbor_table.delay_send_hello(destination)

    def on_frame_received(self, source: NodeId) -> None:
        self._neighbor_table.delay_expire(source)
