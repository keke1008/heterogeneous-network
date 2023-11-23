import { app, BrowserWindow, ipcMain } from "electron";
import path from "node:path";
import { NetService } from "./net/service";
import { ipcChannelName, IpcMainSignature, IpcWebContentsSignature } from "./ipcChannel";
import { BufferReader, Cost, NodeId, SerialAddress, UdpAddress } from "@core/net";
import { Option, Result } from "oxide.ts";
import { deferred } from "@core/deferred";

process.on("uncaughtException", (err: unknown) => {
    console.error("uncaughtException", err);
});

// The built directory structure
//
// ├─┬─┬ dist
// │ │ └── index.html
// │ │
// │ ├─┬ dist-electron
// │ │ ├── main.js
// │ │ └── preload.js
// │
process.env.DIST = path.join(__dirname, "../dist");
process.env.VITE_PUBLIC = app.isPackaged ? process.env.DIST : path.join(process.env.DIST, "../public");

let win: BrowserWindow | null;
// 🚧 Use ['ENV_NAME'] avoid vite:define plugin - Vite@2.x
const VITE_DEV_SERVER_URL = process.env["VITE_DEV_SERVER_URL"];

function createWindow() {
    win = new BrowserWindow({
        icon: path.join(process.env.VITE_PUBLIC, "electron-vite.svg"),
        webPreferences: {
            preload: path.join(__dirname, "preload.js"),
        },
    });

    win.webContents.openDevTools();

    if (VITE_DEV_SERVER_URL) {
        win.loadURL(VITE_DEV_SERVER_URL);
    } else {
        // win.loadFile('dist/index.html')
        win.loadFile(path.join(process.env.DIST, "index.html"));
    }
}

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        app.quit();
        win = null;
    }
});

app.on("activate", () => {
    // On OS X it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow();
    }
});

interface Deserializable<Instance> {
    new (...args: never[]): Instance;
    deserialize(reader: BufferReader): Result<Instance, unknown>;
}

const deserialize = <D extends Deserializable<InstanceType<D>>>(cls: D, buffer: Uint8Array): InstanceType<D> => {
    const reader = new BufferReader(buffer);
    return cls.deserialize(reader).unwrap();
};

app.whenReady().then(async () => {
    createWindow();

    const typedIpcMain: IpcMainSignature = ipcMain as IpcMainSignature;
    const typedWebContents = () => Option.nonNull(win?.webContents as IpcWebContentsSignature | undefined);

    let deferredNet = deferred<NetService>();

    typedIpcMain.handle(ipcChannelName.net.begin, (_, { localUdpAddress, localSerialAddress }) => {
        if (deferredNet.status !== "pending") {
            deferredNet.value?.end();
            deferredNet = deferred();
        }

        const net = new NetService({
            localUdpAddress: deserialize(UdpAddress, localUdpAddress),
            localSerialAddress: deserialize(SerialAddress, localSerialAddress),
        });
        deferredNet.resolve(net);

        net.onNetStateUpdate((state) => {
            typedWebContents().unwrap().send(ipcChannelName.net.onNetStateUpdate, state.serialize());
        });
    });

    typedIpcMain.handle(ipcChannelName.net.end, () => {
        deferredNet.value?.end();
        deferredNet = deferred();
    });

    typedIpcMain.handle(ipcChannelName.net.connectSerial, async (_, { address, cost, portPath }) => {
        const net = await deferredNet;
        net.connectSerial({
            portPath,
            address: deserialize(SerialAddress, address),
            cost: deserialize(Cost, cost),
        });
    });

    typedIpcMain.handle(ipcChannelName.net.connectUdp, async (_, { address, cost }) => {
        const net = await deferredNet;
        net.connectUdp({
            address: deserialize(UdpAddress, address),
            cost: deserialize(Cost, cost),
        });
    });

    typedIpcMain.handle(ipcChannelName.net.syncNetState, async () => {
        const net = await deferredNet;
        return net.syncNetState().serialize();
    });

    typedIpcMain.handle(ipcChannelName.net.rpc.blink, async (_, { target, operation }) => {
        const net = await deferredNet;
        return net.rpc().requestBlink(deserialize(NodeId, target), operation);
    });
});
