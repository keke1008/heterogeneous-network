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
from py_core.ipc import Socket, SocketProtocol, SocketProtocolValue
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
        packet = PostingPacket(Utf8(message))
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


def read_file(file: str) -> str | None:
    """
    ファイルを読み込む
    """
    try:
        with open(file, "r") as f:
            return f.read()
    except Exception as e:
        print("failed to read file", e)
        return None


def append_file(file: str, message: str):
    """
    ファイルに追記する
    """
    try:
        with open(file, "a") as f:
            f.write(message)
    except Exception as e:
        print("failed to write file", e)


class PostingSetting:
    """
    Postingの設定
    """

    message_save_file: str | None = None
    """受信したメッセージの保存先"""


async def posting_client(hetero: Hetero, setting: PostingSetting):
    """
    ユーザの操作を受け付ける
    """
    help = """
Usage:
    ? : このヘルプを表示
    h <address> : Helloパケットを<address>に送信(UDP)
    p <id> <message> : <message>をNodeId(SerialAddress(<id>))に送信
    f <id> <file> : <file>の内容をNodeId(SerialAddress(<id>))に送信
    s <file> : 受信したデータの保存先を<file>に設定
    q : 終了

Example:
    # UdpAddress(192.168.0.1:12345)にHelloを送信
    h 192.168.0.1:12345

    # NodeId(SerialAddress(1))に"hello"を送信
    p 1 hello

    # NodeId(SerialAddress(3))にファイル"data.txt"の内容を送信
    f 3 data.txt
"""
    print(help)

    make_remote_address = lambda id: NodeId.from_address(Address(SerialAddress(id)))

    while True:
        try:
            cmd = await ainput("cmd: ")
            if cmd == "?":
                print(help)
            elif cmd.startswith("h "):
                await hello(hetero, cmd[2:])
            elif cmd.startswith("p "):
                id, message = cmd[2:].split(" ", 1)
                await post(hetero, make_remote_address(int(id)), message)
            elif cmd.startswith("f "):
                id, file = cmd[2:].split(" ", 1)
                message = read_file(file)
                if message is not None:
                    await post(hetero, make_remote_address(int(id)), message)
            elif cmd.startswith("s "):
                setting.message_save_file = cmd[2:]
                print(f"save file changed: {setting.message_save_file}")
            elif cmd == "q":
                break
            else:
                print("invalid command")
                continue
        except Exception as e:
            print("failed to execute command", e)
            continue

        # 通信が終了するのを待つ
        await asyncio.sleep(0.5)


async def posting_server(hetero: Hetero, setting: PostingSetting):
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
                if setting.message_save_file is not None:
                    append_file(setting.message_save_file, packet.message.value)

                await socket.close()
                await socket.wait_closed()
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

    setting = PostingSetting()
    try:
        server_task = asyncio.create_task(posting_server(hetero, setting))
        await posting_client(hetero, setting)

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
