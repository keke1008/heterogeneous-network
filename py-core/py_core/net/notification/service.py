import asyncio
from dataclasses import dataclass

from py_core.net.node.cluster_id import ClusterId
from py_core.net.node.cost import Cost
from py_core.net.node.node_id import NodeId


@dataclass(frozen=True)
class SelfUpdated:
    cluster_id: ClusterId
    cost: Cost


@dataclass(frozen=True)
class NeighborUpdated:
    neighbor: NodeId
    link_cost: Cost


@dataclass(frozen=True)
class NeighborRemoved:
    node_id: NodeId


@dataclass(frozen=True)
class FrameReceived:
    pass


type LocalNotification = SelfUpdated | NeighborUpdated | NeighborRemoved | FrameReceived


class NotificationService:
    _notifications: asyncio.Queue[LocalNotification]

    def __init__(self):
        self._notifications = asyncio.Queue()

    def notify(self, notification: LocalNotification):
        self._notifications.put_nowait(notification)

    async def get_notification(self) -> LocalNotification:
        return await self._notifications.get()
