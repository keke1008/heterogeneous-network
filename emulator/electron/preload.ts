import { contextBridge, ipcRenderer } from "electron";
import { IpcListenChannelNames, IpcRendererSignature, ipcChannelName } from "./ipcChannel";

const typedIpcRenderer = ipcRenderer as IpcRendererSignature;

const listenWithCancelCallback = <T extends IpcListenChannelNames>(name: T) => {
    return (callback: Parameters<Window["ipc"][T]>[0]) => {
        typedIpcRenderer.on(name, callback);
        return () => {
            typedIpcRenderer.removeListener(name, callback);
        };
    };
};

contextBridge.exposeInMainWorld("ipc", {
    [ipcChannelName.net.begin]: (args) => typedIpcRenderer.invoke(ipcChannelName.net.begin, args),
    [ipcChannelName.net.connectUdp]: (args) => typedIpcRenderer.invoke(ipcChannelName.net.connectUdp, args),
    [ipcChannelName.net.connectSerial]: (args) => typedIpcRenderer.invoke(ipcChannelName.net.connectSerial, args),
    [ipcChannelName.net.end]: () => typedIpcRenderer.invoke(ipcChannelName.net.end),
    [ipcChannelName.net.syncNetState]: () => typedIpcRenderer.invoke(ipcChannelName.net.syncNetState),
    [ipcChannelName.net.onNetStateUpdate]: listenWithCancelCallback(ipcChannelName.net.onNetStateUpdate),
    [ipcChannelName.net.rpc.blink]: (args) => typedIpcRenderer.invoke(ipcChannelName.net.rpc.blink, args),
} satisfies Window["ipc"]);
