import { BufferWriter } from "@core/net";
import { contextBridge, ipcRenderer } from "electron";
import { ipcChannelName } from "./ipcChannel";

interface Serializable {
    serializedLength(): number;
    serialize(writer: BufferWriter): void;
}

const serializable = (obj: object): obj is Serializable => {
    if (!("serializedLength" in obj && "serialize" in obj)) {
        return false;
    }
    if (typeof obj.serializedLength !== "function" || typeof obj.serialize !== "function") {
        return false;
    }
    if (obj.serializedLength.length !== 0 || obj.serialize.length !== 1) {
        return false;
    }

    return true;
};

type Serialized<T> = T extends Serializable ? Uint8Array : T extends object ? { [K in keyof T]: Serialized<T[K]> } : T;

const serializeObject = <T extends object>(obj: T): Serialized<T> => {
    const result: Record<string, unknown> = {};

    for (const [key, value] of Object.entries(obj)) {
        if (typeof value === "object") {
            result[key] = serializeObject(value as Record<string, unknown>);
        } else if (serializable(value)) {
            const writer = new BufferWriter(new Uint8Array(value.serializedLength()));
            value.serialize(writer);
            result[key] = writer.unwrapBuffer();
        } else {
            result[key] = value;
        }
    }
    return result as Serialized<T>;
};

contextBridge.exposeInMainWorld("ipc", {
    net: {
        begin: (args) => ipcRenderer.invoke(ipcChannelName.net.begin, args),
        connectUdp: (args) => ipcRenderer.invoke(ipcChannelName.net.connectUdp, args),
        connectSerial: (args) => ipcRenderer.invoke(ipcChannelName.net.connectSerial, args),
        end: () => ipcRenderer.invoke(ipcChannelName.net.end),
        onGraphModified: (callback) => ipcRenderer.on(ipcChannelName.net.onGraphModified, callback),
    },
} satisfies Window["ipc"]);
