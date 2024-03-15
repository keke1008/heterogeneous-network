from dataclasses import dataclass
from enum import Enum

from py_core.net import Address, Cost, NodeId, TunnelPort
from py_core.serde import UInt32, Utf8, Variant, DeriveSerde, RemainingBytes
from py_core.serde.enum import EnumSerde


@dataclass(frozen=True)
class MessageDescriptor(DeriveSerde):
    id: UInt32


@dataclass
class SendHello(DeriveSerde):
    """
    Response: OperationSuccess | OperationFailure
    """

    descriptor: MessageDescriptor
    address: Address
    link_cost: Cost


@dataclass(frozen=True)
class ServerDescriptor(DeriveSerde):
    id: UInt32


@dataclass(frozen=True)
class SocketDescriptor(DeriveSerde):
    id: UInt32


class SocketProtocolValue(Enum):
    TUNNEL = 1
    TRUSTED = 2
    STREAM = 3


@dataclass(frozen=True)
class SocketProtocol(EnumSerde[SocketProtocolValue]):
    @staticmethod
    def enum_type():
        return SocketProtocolValue


@dataclass
class StartServer(DeriveSerde):
    """
    Response: ServerOpened | OperationFailure
    """

    descriptor: MessageDescriptor
    protocol: SocketProtocol
    port: TunnelPort


@dataclass
class CloseServer(DeriveSerde):
    """
    Response: OperationSuccess | OperationFailure
    """

    descriptor: MessageDescriptor
    server: ServerDescriptor


@dataclass
class ConnectSocket(DeriveSerde):
    """
    Response: SocketConnected | OperationFailure
    """

    descriptor: MessageDescriptor
    protocol: SocketProtocol
    remote: NodeId
    port: TunnelPort


@dataclass
class SendData(DeriveSerde):
    """
    Response: OperationSuccess | OperationFailure
    """

    descriptor: MessageDescriptor
    socket: SocketDescriptor
    payload: RemainingBytes


@dataclass
class CloseSocket(DeriveSerde):
    """
    Response: OperationSuccess | OperationFailure
    """

    descriptor: MessageDescriptor
    socket: SocketDescriptor


@dataclass
class Terminate(DeriveSerde):
    pass


RequestBodyTypes = (
    SendHello
    | StartServer
    | CloseServer
    | ConnectSocket
    | CloseSocket
    | SendData
    | Terminate
)


@dataclass(frozen=True)
class RequestMessage(Variant[RequestBodyTypes]):
    @staticmethod
    def variant_types():
        return list(RequestBodyTypes.__args__)


@dataclass
class OperationSuccess(DeriveSerde):
    descriptor: MessageDescriptor


@dataclass(frozen=True)
class TraceStr(Utf8):
    @staticmethod
    def length_type():
        return UInt32


@dataclass
class OperationFailure(DeriveSerde):
    descriptor: MessageDescriptor
    trace: TraceStr


@dataclass
class ServerStarted(DeriveSerde):
    descriptor: MessageDescriptor
    server: ServerDescriptor


@dataclass
class ServerClosed(DeriveSerde):
    server: ServerDescriptor


@dataclass
class ServerSocketConnected(DeriveSerde):
    server: ServerDescriptor
    socket: SocketDescriptor
    protocol: SocketProtocol
    remote: NodeId
    port: TunnelPort


@dataclass
class SocketConnected(DeriveSerde):
    descriptor: MessageDescriptor
    socket: SocketDescriptor
    protocol: SocketProtocol
    remote: NodeId
    port: TunnelPort


@dataclass
class SocketReceived(DeriveSerde):
    socket: SocketDescriptor
    payload: RemainingBytes


@dataclass
class SocketClosed(DeriveSerde):
    socket: SocketDescriptor


ResponseBodyTypes = (
    OperationSuccess
    | OperationFailure
    | ServerStarted
    | ServerClosed
    | ServerSocketConnected
    | SocketConnected
    | SocketReceived
    | SocketClosed
)


@dataclass(frozen=True)
class ResponseMessage(Variant[ResponseBodyTypes]):
    @staticmethod
    def variant_types():
        return list(ResponseBodyTypes.__args__)


type ServerEvent = ServerSocketConnected | ServerClosed

type SocketEvent = SocketReceived | SocketClosed
