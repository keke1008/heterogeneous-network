import { BufferReader, BufferWriter, Cost, SerialAddress, UdpAddress } from "@core/net";
import type { ModifyResult } from "./net";
import type { IpcMainInvokeEvent, IpcRendererEvent } from "electron";
import { Result } from "oxide.ts";

type Satisfies<T, U> = T extends U ? T : never;

type SignatureTypeTag = "invoke" | "listen";

export type InvokeSignatureType = {
    type: "invoke";
    args: unknown[];
    serializedArgs: unknown[];
    result: Promise<unknown>;
};

export type ListenSignatureType = {
    type: "listen";
    args: unknown[];
    serializedArgs: unknown[];
};

export type SignatureType = InvokeSignatureType | ListenSignatureType;

export type IpcSignature = Satisfies<
    {
        ["net:begin"]: {
            type: "invoke";
            args: [{ selfUdpAddress: UdpAddress; selfSerialAddress: SerialAddress }];
            serializedArgs: [{ selfUdpAddress: Uint8Array; selfSerialAddress: Uint8Array }];
            result: Promise<void>;
        };
        ["net:connectSerial"]: {
            type: "invoke";
            args: [{ address: SerialAddress }];
            serializedArgs: [{ address: Uint8Array }];
            result: Promise<void>;
        };
        ["net:connectUdp"]: {
            type: "invoke";
            args: [{ address: UdpAddress; cost: Cost }];
            serializedArgs: [{ address: Uint8Array; cost: Uint8Array }];
            result: Promise<void>;
        };
        ["net:end"]: {
            type: "invoke";
            args: [];
            serializedArgs: [];
            result: Promise<void>;
        };
        ["net:onGraphModified"]: {
            type: "listen";
            args: [result: ModifyResult];
            serializedArgs: [result: ModifyResult];
        };
    },
    { [key: string]: SignatureType }
>;

type SignatureTypeToIpcRendererSignature<T extends SignatureType> = T extends { type: "invoke" }
    ? (...args: T["serializedArgs"]) => T["result"]
    : T extends { type: "listen" }
    ? (callback: (event: IpcRendererEvent, ...args: T["serializedArgs"]) => void) => () => void
    : never;

type IpcRendererSignatureType<T = IpcSignature> = T extends SignatureType
    ? SignatureTypeToIpcRendererSignature<T>
    : { [K in keyof T]: IpcRendererSignatureType<T[K]> };

export type IpcRendererSignature = IpcRendererSignatureType;

declare global {
    export interface Window {
        ipc: IpcRendererSignature;
    }
}

type SignatureTypeToIpcMainSignature<T extends SignatureType> = T extends { type: "invoke" }
    ? (event: IpcMainInvokeEvent, ...args: T["serializedArgs"]) => T["result"] | Awaited<T["result"]>
    : T extends { type: "listen" }
    ? (...args: T["serializedArgs"]) => void
    : never;

type IpcMainSignatureType<T = IpcSignature> = T extends SignatureType
    ? SignatureTypeToIpcMainSignature<T>
    : { [K in keyof T]: IpcMainSignatureType<T[K]> };

export type IpcMainSignature = IpcMainSignatureType;

type IpcChannelNamesType<Tag extends SignatureTypeTag, T = IpcSignature> = keyof T extends infer K
    ? K extends keyof T
        ? T[K] extends SignatureType & { type: Tag }
            ? K
            : never
        : never
    : never;

export type IpcChannelNames = IpcChannelNamesType<SignatureTypeTag>;
export type IpcInvokeChannelNames = IpcChannelNamesType<"invoke">;
export type IpcListenChannelNames = IpcChannelNamesType<"listen">;

type NameToNestedObject<T extends string, U = T> = T extends `${infer Head}:${infer Rest}`
    ? { [K in Head]: NameToNestedObject<Rest, U> }
    : { [K in T]: U };

type IpcChannelNameType<T = IpcChannelNames> = (T extends string ? (t: NameToNestedObject<T>) => void : never) extends (
    t: infer I,
) => void
    ? I
    : never;

export const ipcChannelName: IpcChannelNameType = {
    net: {
        begin: "net:begin",
        connectSerial: "net:connectSerial",
        connectUdp: "net:connectUdp",
        end: "net:end",
        onGraphModified: "net:onGraphModified",
    },
} as const;

export type IpcChannelNameToIpcSignature<T extends IpcChannelNames> = IpcSignature[T];

type SignatureTypeToIpcSerializerType<T extends SignatureType> = (...args: T["args"]) => T["serializedArgs"];
type IpcSerializerType = {
    [K in IpcChannelNames]: SignatureTypeToIpcSerializerType<IpcChannelNameToIpcSignature<K>>;
};

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
    ["net:begin"]: ({ selfUdpAddress, selfSerialAddress }) => [
        { selfUdpAddress: serialize(selfUdpAddress), selfSerialAddress: serialize(selfSerialAddress) },
    ],
    ["net:connectUdp"]: ({ address, cost }) => [{ address: serialize(address), cost: serialize(cost) }],
    ["net:connectSerial"]: ({ address }) => [{ address: serialize(address) }],
    ["net:end"]: () => [],
    ["net:onGraphModified"]: (...args) => args,
};

export const withSerialized = <T extends IpcChannelNames, Ret>(
    name: T,
    callback: (...args: IpcChannelNameToIpcSignature<T>["serializedArgs"]) => Ret,
): ((...args: IpcChannelNameToIpcSignature<T>["args"]) => Ret) => {
    const serializer = ipcSerializer[name] as any;
    return (...args: IpcChannelNameToIpcSignature<T>["args"]) => {
        return callback(...serializer(...args));
    };
};

type SignatureTypeToIpcDeserializerType<T extends SignatureType> = T extends { type: "invoke" }
    ? (...args: T["serializedArgs"]) => T["args"]
    : T extends { type: "listen" }
    ? (...args: T["serializedArgs"]) => T["args"]
    : never;

type IpcDeserializerType = {
    [K in IpcChannelNames]: SignatureTypeToIpcDeserializerType<IpcChannelNameToIpcSignature<K>>;
};

interface Deserializable<Instance> {
    new (...args: never[]): Instance;
    deserialize(reader: BufferReader): Result<Instance, unknown>;
}

const deserialize = <D extends Deserializable<InstanceType<D>>>(cls: D, buffer: Uint8Array): InstanceType<D> => {
    const reader = new BufferReader(buffer);
    return cls.deserialize(reader).unwrap();
};

export const ipcDeserializer: IpcDeserializerType = {
    ["net:begin"]: ({ selfSerialAddress: serial, selfUdpAddress: udp }) => [
        { selfSerialAddress: deserialize(SerialAddress, serial), selfUdpAddress: deserialize(UdpAddress, udp) },
    ],
    ["net:connectUdp"]: ({ address, cost }) => [
        { address: deserialize(UdpAddress, address), cost: deserialize(Cost, cost) },
    ],
    ["net:connectSerial"]: ({ address }) => [{ address: deserialize(SerialAddress, address) }],
    ["net:end"]: () => [],
    ["net:onGraphModified"]: (...args) => args,
};

export const withDeserialized = <T extends IpcChannelNames, Event, Ret>(
    name: T,
    callback: (event: Event, ...args: IpcChannelNameToIpcSignature<T>["args"]) => Ret,
): ((event: Event, ...args: IpcChannelNameToIpcSignature<T>["serializedArgs"]) => Ret) => {
    const deserializer = ipcDeserializer[name] as any;
    return (event: Event, ...args: IpcChannelNameToIpcSignature<T>["serializedArgs"]) => {
        return callback(event, ...deserializer(...args));
    };
};
