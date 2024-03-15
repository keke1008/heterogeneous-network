import asyncio
from dataclasses import dataclass
from py_core.ipc.message import (
    CloseServer,
    CloseSocket,
    ConnectSocket,
    MessageDescriptor,
    OperationSuccess,
    RequestMessage,
    ResponseMessage,
    SendData,
    SendHello,
    ServerClosed,
    ServerDescriptor,
    ServerSocketConnected,
    ServerStarted,
    SocketClosed,
    SocketConnected,
    SocketDescriptor,
    SocketProtocol,
    SocketReceived,
    StartServer,
    Terminate,
)
from py_core.ipc.server import IpcServer
from py_core.net.link.address import Address
from py_core.net.node.cost import Cost
from py_core.net.node.node_id import NodeId
from py_core.net.tunnel.port import TunnelPort
from py_core.serde.bytes import RemainingBytes
from py_core.serde.primitives import UInt32


class MessageDescriptorIncrementer:
    _next: int

    def __init__(self, start: int):
        self._next = start

    def next(self) -> MessageDescriptor:
        res = MessageDescriptor(UInt32(self._next))
        self._next += 1
        return res


@dataclass
class InvalidResponse(Exception):
    response: ResponseMessage


class Socket:
    _ipc: IpcServer
    _message_descriptor: MessageDescriptorIncrementer
    _socket: SocketDescriptor

    _protocol: SocketProtocol
    _remote_node: NodeId
    _remote_port: TunnelPort

    _recv: asyncio.Queue[bytes]
    _closed: asyncio.Event

    def __init__(
        self,
        ipc: IpcServer,
        message_descriptor: MessageDescriptorIncrementer,
        socket: SocketDescriptor,
        protocol: SocketProtocol,
        remote_node: NodeId,
        remote_port: TunnelPort,
    ) -> None:
        self._ipc = ipc
        self._message_descriptor = message_descriptor
        self._socket = socket
        self._protocol = protocol
        self._remote_node = remote_node
        self._remote_port = remote_port
        self._recv = asyncio.Queue[bytes]()
        self._closed = asyncio.Event()

        async def handle_events():
            queue = ipc.socket_queue(socket)

            while True:
                event = await queue.get()
                match event:
                    case SocketReceived(_, data):
                        await self._recv.put(data.value)
                    case SocketClosed():
                        self._closed.set()
                        break
                    case _:
                        raise ValueError("Unexpected event")

        asyncio.create_task(handle_events())

    async def send(self, payload: bytes) -> None:
        descriptor = self._message_descriptor.next()
        mes = SendData(descriptor, self._socket, RemainingBytes(payload))
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(descriptor)
        if not isinstance(res.value, OperationSuccess):
            raise InvalidResponse(res)

    async def recv(self) -> bytes:
        return await self._recv.get()

    async def close(self) -> None:
        descriptor = self._message_descriptor.next()
        mes = CloseSocket(descriptor, self._socket)
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(descriptor)
        if not isinstance(res.value, OperationSuccess):
            raise InvalidResponse(res)

        await self._closed.wait()

    async def wait_closed(self) -> None:
        await self._closed.wait()


class Server:
    _ipc: IpcServer
    _message_descriptor: MessageDescriptorIncrementer
    _server: ServerDescriptor

    _protocol: SocketProtocol
    _port: TunnelPort

    _accept: asyncio.Queue[Socket]
    _closed: asyncio.Event

    def __init__(
        self,
        ipc: IpcServer,
        message_descriptor: MessageDescriptorIncrementer,
        server: ServerDescriptor,
        protocol: SocketProtocol,
        port: TunnelPort,
    ) -> None:
        self._ipc = ipc
        self._message_descriptor = message_descriptor
        self._server = server
        self._protocol = protocol
        self._port = port
        self._accept = asyncio.Queue[Socket]()
        self._closed = asyncio.Event()

        async def handle_events():
            queue = ipc.server_queue(server)

            while True:
                event = await queue.get()
                match event:
                    case ServerSocketConnected(_, socket, protocol, remote, port):
                        await self._accept.put(
                            Socket(
                                ipc,
                                self._message_descriptor,
                                socket,
                                protocol,
                                remote,
                                port,
                            )
                        )
                    case ServerClosed():
                        self._closed.set()
                        break
                    case _:
                        raise ValueError("Unexpected event")

        asyncio.create_task(handle_events())

    @property
    def protocol(self) -> SocketProtocol:
        return self._protocol

    @property
    def port(self) -> TunnelPort:
        return self._port

    async def accept(self) -> Socket:
        return await self._accept.get()

    async def close(self) -> None:
        mes = CloseServer(self._message_descriptor.next(), self._server)
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(mes.descriptor)
        if not isinstance(res.value, OperationSuccess):
            raise InvalidResponse(res)

        await self._closed.wait()

    async def wait_closed(self) -> None:
        await self._closed.wait()


class NetCore:
    _ipc: IpcServer
    _message_descriptor = MessageDescriptorIncrementer(2**30)

    def __init__(self, ipc_server_port: int) -> None:
        self._ipc = IpcServer(ipc_server_port)

    async def wait_for_client_connection(self) -> None:
        await self._ipc.wait_for_connection()

    async def send_hello(self, address: Address, link_cost: Cost) -> None:
        descriptor = self._message_descriptor.next()
        mes = SendHello(descriptor=descriptor, address=address, link_cost=link_cost)
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(descriptor)
        if not isinstance(res.value, OperationSuccess):
            raise InvalidResponse(res)

    async def start_server(self, protocol: SocketProtocol, port: TunnelPort) -> Server:
        descriptor = self._message_descriptor.next()
        mes = StartServer(descriptor=descriptor, protocol=protocol, port=port)
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(descriptor)
        if not isinstance(res.value, ServerStarted):
            raise InvalidResponse(res)

        server = res.value.server
        return Server(self._ipc, self._message_descriptor, server, protocol, port)

    async def connect(
        self, protocol: SocketProtocol, remote_node: NodeId, port: TunnelPort
    ) -> Socket:
        descriptor = self._message_descriptor.next()
        mes = ConnectSocket(
            descriptor=descriptor, protocol=protocol, remote=remote_node, port=port
        )
        await self._ipc.send(RequestMessage(mes))

        res = await self._ipc.get_response(descriptor)
        if not isinstance(res.value, SocketConnected):
            raise InvalidResponse(res)

        socket = res.value.socket
        return Socket(
            self._ipc, self._message_descriptor, socket, protocol, remote_node, port
        )

    async def terminate(self) -> None:
        await self._ipc.send(RequestMessage(Terminate()))
        await self._ipc.close()
