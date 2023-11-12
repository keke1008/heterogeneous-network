import { BufferReader, BufferWriter, Cost, SerialAddress, UdpAddress } from "@core/net";
import type { ModifyResult } from "./net";
import type { IpcMainEvent, IpcMainInvokeEvent, IpcRendererEvent } from "electron";
import { Result } from "oxide.ts";

type Satisties<T, U> = T extends U ? T : never;

type IpcType = "invoke" | "send" | "listen";

type IpcChannelTypes = {
    net: {
        begin: "invoke";
        connectSerial: "invoke";
        connectUdp: "invoke";
        end: "invoke";
        onGraphModified: "listen";
    };
};

type SignatureType = {
    invoke: { type: "invoke"; args: unknown[]; serializedArgs: unknown[]; result: Promise<unknown> };
    send: { type: "send"; args: unknown[]; serializedArgs: unknown[]; result: unknown };
    listen: { type: "listen"; callbackArgs: unknown[]; serializedCallbackArgs: unknown[] };
};

type IpcChannelSignatureType<T = IpcChannelTypes> = T extends Record<string, unknown>
    ? { [K in keyof T]: IpcChannelSignatureType<T[K]> }
    : T extends IpcType
    ? SignatureType[T]
    : never;

type Signature = {
    net: {
        begin: {
            type: "invoke";
            args: [{ selfUdpAddress: UdpAddress; selfSerialAddress: SerialAddress }];
            serializedArgs: [{ selfUdpAddress: Uint8Array; selfSerialAddress: Uint8Array }];
            result: Promise<void>;
        };
        connectSerial: {
            type: "invoke";
            args: [{ address: SerialAddress }];
            serializedArgs: [{ address: Uint8Array }];
            result: Promise<void>;
        };
        connectUdp: {
            type: "invoke";
            args: [{ address: UdpAddress; cost: Cost }];
            serializedArgs: [{ address: Uint8Array; cost: Uint8Array }];
            result: Promise<void>;
        };
        end: {
            type: "invoke";
            args: [];
            serializedArgs: [];
            result: Promise<void>;
        };
        onGraphModified: {
            type: "listen";
            callbackArgs: [result: ModifyResult];
            serializedCallbackArgs: [result: ModifyResult];
        };
    };
};

export type IpcChannelSignature = Satisties<Signature, IpcChannelSignatureType>;

type SignatureTypeToIpcRendererSignature<T extends SignatureType[keyof SignatureType]> = T extends { type: "invoke" }
    ? (...args: T["serializedArgs"]) => T["result"]
    : T extends { type: "send" }
    ? (...args: T["serializedArgs"]) => T["result"]
    : T extends { type: "listen" }
    ? (callback: (event: IpcRendererEvent, ...args: T["serializedCallbackArgs"]) => void) => void
    : never;

type IpcRendererSignatureType<T = IpcChannelSignature> = T extends SignatureType[keyof SignatureType]
    ? SignatureTypeToIpcRendererSignature<T>
    : { [K in keyof T]: IpcRendererSignatureType<T[K]> };

export type IpcRendererSignature = IpcRendererSignatureType;

declare global {
    export interface Window {
        ipc: IpcRendererSignature;
    }
}

type SignatureTypeToIpcMainSignature<T extends SignatureType[keyof SignatureType]> = T extends { type: "invoke" }
    ? (event: IpcMainInvokeEvent, ...args: T["serializedArgs"]) => T["result"] | Awaited<T["result"]>
    : T extends { type: "send" }
    ? (event: IpcMainEvent, ...args: T["serializedArgs"]) => T["result"]
    : T extends { type: "listen" }
    ? (...args: T["serializedCallbackArgs"]) => void
    : never;

type IpcMainSignatureType<T = IpcChannelSignature> = T extends SignatureType[keyof SignatureType]
    ? SignatureTypeToIpcMainSignature<T>
    : { [K in keyof T]: IpcMainSignatureType<T[K]> };

export type IpcMainSignature = IpcMainSignatureType;

type NameType<T = IpcChannelTypes> = T extends Record<string, unknown> ? { [K in keyof T]: NameType<T[K]> } : string;

export const ipcChannelName = {
    net: {
        begin: "net:begin",
        connectSerial: "net:connectSerial",
        connectUdp: "net:connectUdp",
        end: "net:end",
        onGraphModified: "net:onGraphModified",
    },
} satisfies NameType;

type SignatureTypeToIpcSerializerType<T extends SignatureType[keyof SignatureType]> = T extends { type: "invoke" }
    ? (...args: T["args"]) => T["serializedArgs"]
    : T extends { type: "send" }
    ? (...args: T["args"]) => T["serializedArgs"]
    : T extends { type: "listen" }
    ? (...args: T["callbackArgs"]) => T["serializedCallbackArgs"]
    : never;

export type IpcSerializerType<T = IpcChannelSignature> = T extends SignatureType[keyof SignatureType]
    ? SignatureTypeToIpcSerializerType<T>
    : { [K in keyof T]: IpcSerializerType<T[K]> };

interface Serializable {
    serializedLength(): number;
    serialize(writer: BufferWriter): void;
}

const serialize = <T extends Serializable>(obj: T): Uint8Array => {
    const writer = new BufferWriter(new Uint8Array(obj.serializedLength()));
    obj.serialize(writer);
    return writer.unwrapBuffer();
};

export const ipcSerializer: IpcSerializerType = {
    net: {
        begin: ({ selfUdpAddress, selfSerialAddress }) => [
            { selfUdpAddress: serialize(selfUdpAddress), selfSerialAddress: serialize(selfSerialAddress) },
        ],
        connectUdp: ({ address, cost }) => [{ address: serialize(address), cost: serialize(cost) }],
        connectSerial: ({ address }) => [{ address: serialize(address) }],
        end: () => [],
        onGraphModified: (result) => [result],
    },
};

type SignatureTypeToIpcDeserializerType<T extends SignatureType[keyof SignatureType]> = T extends { type: "invoke" }
    ? (...args: T["serializedArgs"]) => T["args"]
    : T extends { type: "send" }
    ? (...args: T["serializedArgs"]) => T["args"]
    : T extends { type: "listen" }
    ? (...args: T["serializedCallbackArgs"]) => T["callbackArgs"]
    : never;

export type IpcDeserializerType<T = IpcChannelSignature> = T extends SignatureType[keyof SignatureType]
    ? SignatureTypeToIpcDeserializerType<T>
    : { [K in keyof T]: IpcDeserializerType<T[K]> };

interface Deserializable<Instance> {
    new (...args: never[]): Instance;
    deserialize(reader: BufferReader): Result<Instance, unknown>;
}

const deserialize = <D extends Deserializable<InstanceType<D>>>(cls: D, buffer: Uint8Array): InstanceType<D> => {
    const reader = new BufferReader(buffer);
    return cls.deserialize(reader).unwrap();
};

export const ipcDeserializer: IpcDeserializerType = {
    net: {
        begin: ({ selfSerialAddress: serial, selfUdpAddress: udp }) => [
            { selfSerialAddress: deserialize(SerialAddress, serial), selfUdpAddress: deserialize(UdpAddress, udp) },
        ],
        connectUdp: ({ address, cost }) => [
            { address: deserialize(UdpAddress, address), cost: deserialize(Cost, cost) },
        ],
        connectSerial: ({ address }) => [{ address: deserialize(SerialAddress, address) }],
        end: () => [],
        onGraphModified: (result) => [result],
    },
};
