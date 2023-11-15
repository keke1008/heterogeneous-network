import { Cost, SerialAddress, UdpAddress } from "@core/net";
import { StateUpdate, SerializedStateUpdate } from "./net/linkState";
import type { IpcMainInvokeEvent, IpcRendererEvent } from "electron";
import { UnionToIntersection } from "@core/types";

type Satisfies<T, U> = T extends U ? T : never;

type SignatureTypeTag = "invoke" | "listen";

export type InvokeSignatureType = {
    type: "invoke";
    args: unknown[];
    serializedArgs: unknown[];
    result: Promise<unknown>;
    serializedResult: Promise<unknown>;
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
            args: [{ localUdpAddress: UdpAddress; localSerialAddress: SerialAddress }];
            serializedArgs: [{ localUdpAddress: Uint8Array; localSerialAddress: Uint8Array }];
            result: Promise<void>;
            serializedResult: Promise<void>;
        };
        ["net:connectSerial"]: {
            type: "invoke";
            args: [{ address: SerialAddress; cost: Cost; portPath: string }];
            serializedArgs: [{ address: Uint8Array; cost: Uint8Array; portPath: string }];
            result: Promise<void>;
            serializedResult: Promise<void>;
        };
        ["net:connectUdp"]: {
            type: "invoke";
            args: [{ address: UdpAddress; cost: Cost }];
            serializedArgs: [{ address: Uint8Array; cost: Uint8Array }];
            result: Promise<void>;
            serializedResult: Promise<void>;
        };
        ["net:end"]: {
            type: "invoke";
            args: [];
            serializedArgs: [];
            result: Promise<void>;
            serializedResult: Promise<void>;
        };
        ["net:onNetStateUpdate"]: {
            type: "listen";
            args: [update: StateUpdate];
            serializedArgs: [result: SerializedStateUpdate];
        };
        ["net:syncNetState"]: {
            type: "invoke";
            args: [];
            serializedArgs: [];
            result: Promise<StateUpdate>;
            serializedResult: Promise<SerializedStateUpdate>;
        };
    },
    { [key: string]: SignatureType }
>;

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
        syncNetState: "net:syncNetState",
        onNetStateUpdate: "net:onNetStateUpdate",
    },
} as const;

export type IpcChannelNameToIpcSignature<T extends IpcChannelNames> = IpcSignature[T];

type SignatureTypeToIpcWindowSignature<T extends SignatureType> = T extends { type: "invoke" }
    ? (...args: T["serializedArgs"]) => T["serializedResult"]
    : T extends { type: "listen" }
    ? (callback: (event: IpcRendererEvent, ...args: T["serializedArgs"]) => void) => () => void
    : never;

type IpcWindowSignatureType<T = IpcSignature> = T extends SignatureType
    ? SignatureTypeToIpcWindowSignature<T>
    : { [K in keyof T]: IpcWindowSignatureType<T[K]> };

export type IpcWindowSignature = IpcWindowSignatureType;

declare global {
    export interface Window {
        ipc: IpcWindowSignature;
    }
}

type ChannelNameToIpcRendererSignatureType<
    C extends IpcChannelNames,
    Signature extends IpcChannelNameToIpcSignature<C>,
> = Signature extends { type: "invoke" }
    ? { invoke: (channel: C, ...args: Signature["serializedArgs"]) => Signature["serializedResult"] }
    : Signature extends { type: "listen" }
    ? {
          on: (channel: C, callback: (event: IpcRendererEvent, ...args: Signature["serializedArgs"]) => void) => void;
          removeListener: (
              channel: C,
              callback: (event: IpcRendererEvent, ...args: Signature["serializedArgs"]) => void,
          ) => void;
      }
    : never;

type IpcRendererSignatureType<C extends IpcChannelNames = IpcChannelNames> = C extends unknown
    ? ChannelNameToIpcRendererSignatureType<C, IpcChannelNameToIpcSignature<C>>
    : never;

export type IpcRendererSignature = UnionToIntersection<IpcRendererSignatureType>;

type ChannelNameToHandleSignature<
    C extends IpcInvokeChannelNames,
    Signature extends InvokeSignatureType = IpcChannelNameToIpcSignature<C>,
> = (
    channel: C,
    listener: (
        event: IpcMainInvokeEvent,
        ...args: Signature["serializedArgs"]
    ) => Signature["serializedResult"] | Awaited<Signature["serializedResult"]>,
) => void;

type IpcMainSignatureType<C extends IpcInvokeChannelNames = IpcInvokeChannelNames> = C extends unknown
    ? { handle: ChannelNameToHandleSignature<C>; handleOnce: ChannelNameToHandleSignature<C> }
    : never;

export type IpcMainSignature = UnionToIntersection<IpcMainSignatureType>;

type IpcWebContentsSignatureType<C extends IpcListenChannelNames = IpcListenChannelNames> = {
    send: (channel: C, ...args: IpcChannelNameToIpcSignature<C>["serializedArgs"]) => void;
};

export type IpcWebContentsSignature = UnionToIntersection<IpcWebContentsSignatureType>;
