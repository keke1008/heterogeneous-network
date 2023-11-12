import { app, BrowserWindow, ipcMain } from "electron";
import path from "node:path";
import { NetService } from "./net";
import { ipcChannelName, ipcDeserializer, IpcMainSignature } from "./ipcChannel";
import { AddressError, BufferReader, SerialAddress, UdpAddress } from "@core/net";

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

app.whenReady().then(() => {
    createWindow();

    const net = new NetService();

    ipcMain.handle(ipcChannelName.net.begin, ((_, args) => {
        const deserialized = ipcDeserializer.net.begin(args);
        net.begin(deserialized);

        net.onGraphModified((result) => {
            const deserialized = ipcDeserializer.net.onGraphModified(result);
            win?.webContents.send(ipcChannelName.net.onGraphModified, deserialized);
        });
    }) satisfies IpcMainSignature["net"]["begin"]);

    ipcMain.handle(ipcChannelName.net.connectSerial, ((_, args) => {
        const deserialized = ipcDeserializer.net.connectSerial(args);
        net.connectSerial(deserialized);
    }) satisfies IpcMainSignature["net"]["connectSerial"]);

    ipcMain.handle(ipcChannelName.net.connectUdp, ((_, args) => {
        const deserialized = ipcDeserializer.net.connectUdp(args);
        net.connectUdp(deserialized);
    }) satisfies IpcMainSignature["net"]["connectUdp"]);

    ipcMain.on(ipcChannelName.net.end, () => net.end());
});
