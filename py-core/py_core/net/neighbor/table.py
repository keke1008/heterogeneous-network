import asyncio
from typing import Protocol

from py_core.net.link.address import Address, AddressType, LoopbackAddress
from py_core.net.local.service import NodeInfo
from py_core.net.neighbor.constants import (
    NEIGHBOR_EXPIRATION_TIMEOUT,
    SEND_HELLO_INTERVAL,
)
from py_core.net.node import NodeId, Cost
from py_core.serde.primitives import UInt16


class NeighborExpiration:
    _expire_event: asyncio.Event
    _expire_task: asyncio.Task | None

    def __init__(self):
        self._expire_event = asyncio.Event()
        self._expire_task = asyncio.create_task(self._expire())

    @property
    def event(self) -> asyncio.Event:
        return self._expire_event

    async def _expire(self) -> None:
        try:
            await asyncio.sleep(NEIGHBOR_EXPIRATION_TIMEOUT.seconds)
            self._expire_event.set()
        except asyncio.CancelledError:
            pass

    def delay(self) -> None:
        if self._expire_task is not None:
            self._expire_task.cancel()
            self._expire_task = asyncio.create_task(self._expire())

    def cancel(self) -> None:
        if self._expire_task is not None:
            self._expire_task.cancel()
            self._expire_task = None


class NeighborSendHelloInterval:
    _send_hello_event: asyncio.Event
    _expire_event: asyncio.Event
    _send_hello_task: asyncio.Task

    async def _emit_send_hello(self):
        try:
            while True:
                await asyncio.sleep(SEND_HELLO_INTERVAL.seconds)
                if self._expire_event.is_set():
                    break

                self._send_hello_event.set()
                self._send_hello_event.clear()
        except asyncio.CancelledError:
            pass

    def __init__(self, expire_event: asyncio.Event) -> None:
        self._send_hello_event = asyncio.Event()
        self._expire_event = expire_event
        self._send_hello_task = asyncio.create_task(self._emit_send_hello())

    @property
    def event(self) -> asyncio.Event:
        return self._send_hello_event

    def delay(self) -> None:
        self._send_hello_task.cancel()
        self._send_hello_task = asyncio.create_task(self._emit_send_hello())


class NeighborNode:
    node_id: NodeId
    link_cost: Cost
    addresses: set[Address]
    expiration: NeighborExpiration
    send_hello_interval: NeighborSendHelloInterval

    def __init__(self, node_id: NodeId, link_cost: Cost):
        self.node_id = node_id
        self.link_cost = link_cost
        self.addresses = set()
        self.expiration = NeighborExpiration()
        self.send_hello_interval = NeighborSendHelloInterval(self.expiration.event)

    @property
    def expire_event(self) -> asyncio.Event:
        return self.expiration.event

    @property
    def send_hello_event(self) -> asyncio.Event:
        return self.send_hello_interval.event

    def add_address(self, address: Address):
        self.addresses.add(address)

    def has_address_type(self, address_type: AddressType) -> bool:
        return any(address.type == address_type for address in self.addresses)

    def cancel_expire(self):
        self.expiration.cancel()

    def delay_expire(self):
        self.expiration.delay()

    def delay_send_hello(self):
        self.send_hello_interval.delay()


class NeighborTableListener(Protocol):
    def on_neighbor_updated(self, neighbor: NeighborNode) -> None: ...

    def on_neighbor_removed(self, neighbor: NeighborNode) -> None: ...

    def on_neighbor_should_send_hello(self, neighbor: NeighborNode) -> None: ...


class _ListenerList:
    _listeners: list[NeighborTableListener]

    def __init__(self):
        self._listeners = []

    def add(self, listener: NeighborTableListener) -> None:
        self._listeners.append(listener)

    def on_neighbor_updated(self, neighbor: NeighborNode) -> None:
        for listener in self._listeners:
            listener.on_neighbor_updated(neighbor)

    def on_neighbor_removed(self, neighbor: NeighborNode) -> None:
        for listener in self._listeners:
            listener.on_neighbor_removed(neighbor)

    def on_neighbor_should_send_hello(self, neighbor: NeighborNode) -> None:
        for listener in self._listeners:
            listener.on_neighbor_should_send_hello(neighbor)


class NeighborTable:
    _neighbors: dict[NodeId, NeighborNode]
    _listeners: _ListenerList

    def __init__(self):
        self._neighbors = {}
        self._listeners = _ListenerList()

        loopback = NeighborNode(NodeId.loopback(), Cost(UInt16(0)))
        loopback.cancel_expire()
        loopback.add_address(LoopbackAddress())
        self._neighbors[NodeId.loopback()] = loopback

    def initialize_local_node(self, info: NodeInfo) -> None:
        node_id = info.source.node_id
        if node_id in self._neighbors:
            raise ValueError("Local node already initialized")

        local_node = NeighborNode(node_id, info.cost)
        local_node.cancel_expire()
        local_node.add_address(LoopbackAddress())
        self._neighbors[node_id] = local_node

    def add_listener(self, listener: NeighborTableListener) -> None:
        self._listeners.add(listener)

    def add_neighbor(self, node_id: NodeId, link_cost: Cost, address: Address) -> None:
        neighbor = self._neighbors.get(node_id)
        if neighbor is None:
            neighbor = NeighborNode(node_id, link_cost)
            self._neighbors[node_id] = neighbor
            self._listeners.on_neighbor_updated(neighbor)

        neighbor.add_address(address)

        async def expire_neighbor():
            await neighbor.expire_event.wait()
            del self._neighbors[node_id]
            self._listeners.on_neighbor_removed(neighbor)

        asyncio.create_task(expire_neighbor())

        async def send_hello():
            while True:
                await neighbor.send_hello_event.wait()
                self._listeners.on_neighbor_should_send_hello(neighbor)

        asyncio.create_task(send_hello())

    def get_neighbor(self, node_id: NodeId) -> NeighborNode | None:
        return self._neighbors.get(node_id)

    def resolve_neighbor_from_address(self, address: Address) -> NeighborNode | None:
        for neighbor in self._neighbors.values():
            if address in neighbor.addresses:
                return neighbor
        return None

    def delay_expire(self, node_id: NodeId) -> None:
        neighbor = self._neighbors.get(node_id)
        if neighbor is not None:
            neighbor.delay_expire()

    def delay_send_hello(self, node_id: NodeId) -> None:
        neighbor = self._neighbors.get(node_id)
        if neighbor is not None:
            neighbor.delay_send_hello()
