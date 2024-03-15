import { P, match } from "ts-pattern";
import { IpcSocket } from "./ipc";
import { NetCore } from "./net";
import {
    CloseServer,
    CloseSocket,
    ConnectSocket,
    MessageDescriptor,
    ResponseMessage,
    SendData,
    SendHello,
    StartServer,
    Terminate,
} from "./message";

const parseArgs = () => {
    const portStr = process.argv[2];
    if (portStr === undefined) {
        console.error("Expected port number as first argument");
        process.exit(1);
    }

    const port = parseInt(portStr);
    if (isNaN(port)) {
        console.error("Expected port number as first argument");
        process.exit(1);
    }

    return port;
};

const main = async () => {
    const port = parseArgs();
    const res = await IpcSocket.connect(port);
    if (res.isErr()) {
        console.error("Failed to connect to socket", res.unwrapErr());
        process.exit(1);
    }
    const socket = res.unwrap();

    const net = new NetCore();
    {
        let nextDescriptor = 0;
        net.listenResponse((body) => {
            socket.send(new ResponseMessage({ descriptor: new MessageDescriptor(nextDescriptor++), body }));
        });
    }

    socket.onMessage(async (message) => {
        const res = await match(message.body)
            .with(P.instanceOf(SendHello), (m) => net.sendHello(m))
            .with(P.instanceOf(StartServer), (m) => net.startServer(m))
            .with(P.instanceOf(CloseServer), (m) => net.closeServer(m))
            .with(P.instanceOf(ConnectSocket), (m) => net.connectSocket(m))
            .with(P.instanceOf(CloseSocket), (m) => net.closeSocket(m))
            .with(P.instanceOf(SendData), (m) => net.sendData(m))
            .with(P.instanceOf(Terminate), () => "terminate" as const)
            .exhaustive();

        if (res === "terminate") {
            socket.close();
            process.exit(0);
        }

        socket.send(new ResponseMessage({ descriptor: message.descriptor, body: res }));
    });
};

main();
