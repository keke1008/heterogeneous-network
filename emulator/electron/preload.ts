import { contextBridge, ipcRenderer } from "electron";
import { IpcListenChannelNames, ipcChannelName } from "./ipcChannel";

const listenWithCallback = <T extends IpcListenChannelNames>(name: T) => {
    return (callback: Parameters<Window["ipc"][T]>[0]) => {
        ipcRenderer.on(name, callback);
        return () => {
            ipcRenderer.removeListener(name, callback);
        };
    };
};

contextBridge.exposeInMainWorld("ipc", {
    ["net:begin"]: (args) => ipcRenderer.invoke(ipcChannelName.net.begin, args),
    ["net:connectUdp"]: (args) => ipcRenderer.invoke(ipcChannelName.net.connectUdp, args),
    ["net:connectSerial"]: (args) => ipcRenderer.invoke(ipcChannelName.net.connectSerial, args),
    ["net:end"]: () => ipcRenderer.invoke(ipcChannelName.net.end),
    ["net:onNetStateUpdate"]: listenWithCallback(ipcChannelName.net.onNetStateUpdate),
} satisfies Window["ipc"]);
