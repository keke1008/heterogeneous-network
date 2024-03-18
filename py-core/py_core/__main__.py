"""
py-coreモジュールの使用方法の例

この例では，Postingの実装例を示す．
"""

import sys
from pathlib import Path

# モジュールのパスを追加する
root = Path(__file__).resolve().parent.parent
sys.path.append(str(root))

import asyncio
import sys
from dataclasses import dataclass
from ipaddress import IPv4Address
from py_core import Hetero, start_hetero
from py_core.ipc.core import Socket
from py_core.ipc.message import SocketProtocol, SocketProtocolValue
from py_core.net import Address, TunnelPort, Cost, NodeId, SerialAddress, UdpAddress
from py_core.serde import BufferReader, BufferWriter, UInt16, DeriveSerde, Utf8

POSTING_PROTOCOL = SocketProtocol(SocketProtocolValue.TUNNEL)
POSTING_PORT = TunnelPort(UInt16(104))


@dataclass
class PostingPacket(DeriveSerde):
    message: Utf8


def parse_udp_address(address: str) -> UdpAddress:
    """
    `192.168.0.1:1234`のような文字列をUdpAddressに変換する
    """
    address = address.strip()
    host, port = address.split(":")
    return UdpAddress(host=IPv4Address(host), port=int(port))


async def hello(hetero: Hetero, args: str):
    """
    Helloを送信する
    """
    try:
        server_address = Address(parse_udp_address(args))
    except Exception as e:
        print(f"failed to parse address: {args}", e)
        return

    try:
        await hetero.send_hello(server_address, Cost(UInt16(0)))
    except Exception as e:
        print("failed to send hello", e)


async def post(hetero: Hetero, remote: NodeId, message: str):
    """
    Postingを送信する
    """
    try:
        socket = await hetero.connect(
            protocol=POSTING_PROTOCOL,
            remote_node=remote,
            port=POSTING_PORT,
        )

        # メッセージを送信
        packet = Utf8(message)
        writer = BufferWriter(packet.serialized_length())
        packet.serialize(writer)
        await socket.send(writer.unwrap_buffer())
        await socket.close()
        await socket.wait_closed()

    except Exception as e:
        print("failed to create NodeId", e)
        return


async def ainput(prompt: str) -> str:
    """
    asyncioで標準入力を行う
    """
    await asyncio.to_thread(sys.stdout.write, prompt)
    await asyncio.to_thread(sys.stdout.flush)
    return (await asyncio.to_thread(sys.stdin.readline)).rstrip("\n")


async def posting_client(hetero: Hetero):
    """
    Postingを行うクライアント
    """
    usage = """
Usage:
    ? : help
    h <address> : send hello via UDP
    p <message> : post message
    pp <id> <message> : post message to SerialAddress(<id>)
    q : quit

Example:
    # send hello to UdpAddress(192.168.0.1:12345)
    h 192.168.0.1:12345

    # post message "Hello" to NodeId(SerialAddress(1))
    p hello

    # post message "Hello" to NodeId(SerialAddress(2))
    pp 2 hello
"""
    print(usage)

    make_remote_address = lambda id: NodeId.from_address(Address(SerialAddress(id)))

    while True:
        cmd = await ainput("cmd: ")
        if cmd == "?":
            print(usage)
        elif cmd.startswith("h "):
            await hello(hetero, cmd[2:])
        elif cmd.startswith("p "):
            message = cmd[2:]
            await post(hetero, make_remote_address(1), message)
        elif cmd.startswith("pp "):
            id, message = cmd[3:].split(" ", 1)
            await post(hetero, make_remote_address(int(id)), message)
        elif cmd == "q":
            break
        else:
            print("invalid command")
            continue

        await asyncio.sleep(0.5)


async def posting_server(hetero: Hetero):
    """
    Postingサーバ
    """
    # サーバを起動する
    try:
        server = await hetero.start_server(protocol=POSTING_PROTOCOL, port=POSTING_PORT)
    except Exception as e:
        print("failed to start server", e)
        return

    # クライアントからのメッセージを待つ
    async def recv(socket: Socket):
        while True:
            try:
                writer = BufferReader(await socket.recv())
                packet = PostingPacket.deserialize(writer)
                print(f"Posted: {packet.message.value}")
            except asyncio.CancelledError:
                pass
            except Exception as e:
                print("failed to recv", e)
                return

    async def close(socket: Socket, task: asyncio.Task):
        await socket.wait_closed()
        task.cancel()

    try:
        while True:
            socket = await server.accept()
            recv_task = asyncio.create_task(recv(socket))
            asyncio.create_task(close(socket, recv_task))
    except asyncio.CancelledError:
        pass
    finally:
        await server.close()
        await server.wait_closed()


async def main():
    hetero = await start_hetero()
    await asyncio.sleep(1)  # 他のログが出力されるのを待つ
    try:
        server_task = asyncio.create_task(posting_server(hetero))
        await posting_client(hetero)

        server_task.cancel()
        try:
            await server_task
        except asyncio.CancelledError:
            pass

    except Exception as e:
        print(e)
    finally:
        await hetero.terminate()


if __name__ == "__main__":
    asyncio.run(main())
