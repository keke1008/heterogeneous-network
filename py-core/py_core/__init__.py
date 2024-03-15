import asyncio
from pathlib import Path

import logging

from py_core.ipc import NetCore
from py_core.ipc.message import SocketProtocol
from py_core.net import Address, Cost, NodeId, TunnelPort

IPC_SERVER_PORT = 9876


class Hetero:
    _core: NetCore
    _node: asyncio.subprocess.Process

    def __init__(self, core: NetCore, node: asyncio.subprocess.Process) -> None:
        self._core = core
        self._node = node

    async def send_hello(self, address: Address, link_cost: Cost) -> None:
        return await self._core.send_hello(address, link_cost)

    async def start_server(self, protocol: SocketProtocol, port: TunnelPort):
        return await self._core.start_server(protocol, port)

    async def connect(
        self, protocol: SocketProtocol, remote_node: NodeId, port: TunnelPort
    ):
        return await self._core.connect(protocol, remote_node, port)

    async def terminate(self) -> None:
        await self._core.terminate()
        await self._node.wait()


async def start_hetero() -> Hetero:
    core = NetCore(ipc_server_port=IPC_SERVER_PORT)

    cmd = f"npx tsx src/index.ts {IPC_SERVER_PORT}"
    cwd = Path(__file__).resolve().parent.parent / "node"

    logging.info(f"Starting node with command: {cmd} in directory: {cwd}")
    node = await asyncio.create_subprocess_shell(cmd, cwd=cwd)

    await core.wait_for_client_connection()
    print("Node started")
    return Hetero(core, node)
