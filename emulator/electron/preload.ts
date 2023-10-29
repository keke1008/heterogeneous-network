import { contextBridge, ipcRenderer } from "electron";

contextBridge.exposeInMainWorld("net", {
    begin: (args) => ipcRenderer.send("net:begin", args),
    onGraphModified: (callback) => ipcRenderer.on("net:onGraphModified", callback),
    connect: (args) => ipcRenderer.send("net:connect", args),
    end: () => ipcRenderer.send("net:end"),
} satisfies Window["net"]);
