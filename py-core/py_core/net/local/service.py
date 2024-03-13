import asyncio
from dataclasses import dataclass
from typing import cast
from py_core.net.node.cluster_id import ClusterId

from py_core.net.node.cost import Cost
from py_core.net.node.node_id import NodeId
from py_core.net.node.source import Source
from py_core.net.notification.service import NotificationService, SelfUpdated
from py_core.serde.primitives import UInt16


@dataclass(frozen=True)
class NodeInfo:
    source: Source
    cost: Cost


class NodeInfoLock:
    _node_info: NodeInfo | None
    _set_event: asyncio.Event

    def __init__(self):
        self._node_info = None
        self._set_event = asyncio.Event()

    async def get(self) -> NodeInfo:
        await self._set_event.wait()
        return cast(NodeInfo, self._node_info)

    def set(self, node_info: NodeInfo):
        self._node_info = node_info
        self._set_event.set()

    def has_value(self) -> bool:
        return self._set_event.is_set()


class LocalNodeService:
    _notification_service: NotificationService

    _node_info: NodeInfoLock

    def __init__(self, notification_service: NotificationService):
        self._notification_service = notification_service
        self._node_info = NodeInfoLock()

    async def info(self) -> NodeInfo:
        return await self._node_info.get()

    async def node_id(self) -> NodeId:
        return (await self.info()).source.node_id

    async def cost(self) -> Cost:
        return (await self.info()).cost

    async def is_local_node_like_id(self, node_id: NodeId) -> bool:
        return node_id.is_loopback() or (await self.node_id()) == node_id

    async def convert_local_node_id_to_loopback(self, node_id: NodeId) -> NodeId:
        if (await self.node_id()) == node_id:
            return NodeId.loopback()
        else:
            return node_id

    def initialize(
        self,
        node_id: NodeId,
        cluster_id: ClusterId = ClusterId.no_cluster(),
        cost: Cost = Cost(UInt16(0)),
    ):
        if self._node_info.has_value():
            raise ValueError("NodeInfo already initialized")
        self._node_info.set(NodeInfo(Source(node_id, cluster_id), cost))

    async def set_cost(self, cost: Cost):
        info = await self.info()
        if info.cost != cost:
            self._node_info.set(NodeInfo(info.source, cost))
            self._notification_service.notify(SelfUpdated(info.source.cluster_id, cost))

    async def set_cluster_id(self, cluster_id: ClusterId):
        info = await self.info()
        if info.source.cluster_id != cluster_id:
            source = Source(info.source.node_id, cluster_id)
            self._node_info.set(NodeInfo(source, info.cost))
            self._notification_service.notify(SelfUpdated(cluster_id, info.cost))
