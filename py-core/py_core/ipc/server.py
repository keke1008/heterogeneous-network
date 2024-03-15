import asyncio
from dataclasses import dataclass

import websockets

from py_core.ipc.message import (
    MessageDescriptor,
    OperationFailure,
    OperationSuccess,
    RequestMessage,
    ResponseMessage,
    ServerClosed,
    ServerDescriptor,
    ServerEvent,
    ServerSocketConnected,
    ServerStarted,
    SocketClosed,
    SocketConnected,
    SocketDescriptor,
    SocketEvent,
    SocketReceived,
    Terminate,
)
from py_core.serde.buffer import BufferReader, BufferWriter


class OneShot[T]:
    _value: T | None
    _event: asyncio.Event

    def __init__(self):
        self._value = None
        self._event = asyncio.Event()

    async def get(self) -> T:
        await self._event.wait()
        if self._value is None:
            raise ValueError("Not set")
        return self._value

    def set(self, value: T):
        if self._value is not None:
            raise ValueError("Already set")
        self._value = value
        self._event.set()

    def has_value(self) -> bool:
        return self._value is not None


@dataclass
class Connection:
    _conn: websockets.WebSocketServerProtocol

    async def send(self, message: RequestMessage) -> None:
        writer = BufferWriter(message.serialized_length())
        message.serialize(writer)
        await self._conn.send(writer.unwrap_buffer())


class IpcServer:
    _response: dict[MessageDescriptor, OneShot[ResponseMessage]]
    _server: dict[ServerDescriptor, asyncio.Queue[ServerEvent]]
    _socket: dict[SocketDescriptor, asyncio.Queue[SocketEvent]]

    _connection: OneShot[Connection]
    _ws_server: websockets.WebSocketServer

    async def _handler(self, ws: websockets.WebSocketServerProtocol):
        if self._connection.has_value():
            return

        self._connection.set(Connection(ws))

        async for message in ws:
            if not isinstance(message, bytes):
                continue

            reader = BufferReader(message)
            response = ResponseMessage.deserialize(reader)

            match response.value:
                case (
                    OperationSuccess(descriptor)
                    | OperationFailure(descriptor)
                    | ServerStarted(descriptor)
                    | SocketConnected(descriptor)
                ):
                    if descriptor in self._response:
                        self._response[descriptor].set(response)
                        del self._response[descriptor]
                    if isinstance(response.value, ServerStarted):
                        self._server[response.value.server] = asyncio.Queue()
                    if isinstance(response.value, SocketConnected):
                        self._socket[response.value.socket] = asyncio.Queue()
                case ServerSocketConnected(server) | ServerClosed(server):
                    if server in self._server:
                        await self._server[server].put(response.value)
                        if isinstance(response.value, ServerClosed):
                            del self._server[server]
                case SocketReceived(socket) | SocketClosed(socket):
                    if socket in self._socket:
                        await self._socket[socket].put(response.value)
                        if isinstance(response.value, SocketClosed):
                            del self._socket[socket]
                case _:
                    raise ValueError("Unexpected response", response.value)

    def __init__(self, port: int):
        self._response = {}
        self._server = {}
        self._socket = {}
        self._connection = OneShot()

        async def serve():
            async with websockets.serve(self._handler, "localhost", port) as serve:
                self._ws_server = serve
                await self._ws_server.wait_closed()

        asyncio.create_task(serve())

    async def wait_for_connection(self) -> None:
        await self._connection.get()

    async def close(self) -> None:
        self._ws_server.close()
        await self._ws_server.wait_closed()

    async def send(self, message: RequestMessage):
        if not isinstance(message.value, Terminate):
            self._response[message.value.descriptor] = OneShot[ResponseMessage]()
        conn = await self._connection.get()
        await conn.send(message)

    async def get_response(self, descriptor: MessageDescriptor) -> ResponseMessage:
        if descriptor not in self._response:
            raise ValueError("Descriptor not found")
        return await self._response[descriptor].get()

    def server_queue(self, server: ServerDescriptor) -> asyncio.Queue[ServerEvent]:
        if server not in self._server:
            raise ValueError("Server not found")
        return self._server[server]

    def socket_queue(self, socket: SocketDescriptor) -> asyncio.Queue[SocketEvent]:
        if socket not in self._socket:
            raise ValueError("Socket not found")
        return self._socket[socket]
