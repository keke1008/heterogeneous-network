import {
    InvokeSignatureType,
    IpcInvokeChannelNames,
    IpcListenChannelNames,
    IpcSignature,
    ipcChannelName,
} from "emulator/electron/ipcChannel";
import { UnionToIntersection } from "@core/types";
import { useEffect } from "react";
import { BufferWriter } from "@core/net";
import { StateUpdate } from "emulator/electron/net/linkState";

export type InvokeOptions<T> = {
    onSuccess?: (value: Awaited<T>) => void;
    onError?: (error: unknown) => void;
};

type SingleInvokeHook<T extends IpcInvokeChannelNames> = T extends IpcInvokeChannelNames
    ? {
          useInvoke: (
              opts?: InvokeOptions<IpcSignature[T]["result"]>,
          ) => (...args: IpcSignature[T]["args"]) => IpcSignature[T]["result"];
      }
    : never;

type ToInvokeChannelName<T extends string> = T extends IpcInvokeChannelNames ? T : never;

type UnionInvokeHooks<
    Name extends string = IpcInvokeChannelNames,
    Init extends string = "",
> = Name extends `${infer Head}:${infer Rest}`
    ? { [K in Head]: UnionInvokeHooks<Rest, `${Init}${Head}:`> }
    : { [K in Name]: SingleInvokeHook<ToInvokeChannelName<`${Init}${Name}`>> };

type IpcInvokeHooks = UnionToIntersection<UnionInvokeHooks>;

type SingleListenHook<T extends string> = T extends IpcListenChannelNames
    ? { useListen: (callback: (...args: IpcSignature[T]["args"]) => void) => void }
    : never;

type UnionListenHooks<
    Name extends string = IpcListenChannelNames,
    Init extends string = "",
> = Name extends `${infer Head}:${infer Rest}`
    ? { [K in Head]: UnionListenHooks<Rest, `${Init}${Head}:`> }
    : { [K in Name]: SingleListenHook<`${Init}${Name}`> };

type IpcListenHooks = UnionToIntersection<UnionListenHooks>;

interface Serializable {
    serializedLength(): number;
    serialize(writer: BufferWriter): void;
}

const serialize = <T extends Serializable>(obj: T): Uint8Array => {
    const writer = new BufferWriter(new Uint8Array(obj.serializedLength()));
    obj.serialize(writer);
    return writer.unwrapBuffer();
};

const withInvoke = <T extends IpcInvokeChannelNames, Signature extends InvokeSignatureType = IpcSignature[T]>(
    f: (...args: Signature["args"]) => Signature["result"],
) => ({
    useInvoke: (opts?: InvokeOptions<Signature["result"]>) => {
        return async (...args: Signature["args"]): Promise<Awaited<Signature["result"]>> => {
            try {
                const result = await f(...args);
                opts?.onSuccess?.(result);
                return result;
            } catch (error) {
                opts?.onError?.(error);
                throw error;
            }
        };
    },
});

const withListen = <T extends IpcListenChannelNames>(
    f: (callback: (...args: IpcSignature[T]["args"]) => void) => () => void,
) => ({
    useListen: (callback: (...args: IpcSignature[T]["args"]) => void) => {
        useEffect(() => {
            return f(callback);
        }, [callback]);
    },
});

export const ipc: IpcInvokeHooks & IpcListenHooks = {
    net: {
        begin: withInvoke<typeof ipcChannelName.net.begin>(({ localUdpAddress, localSerialAddress }) => {
            return window.ipc[ipcChannelName.net.begin]({
                localUdpAddress: serialize(localUdpAddress),
                localSerialAddress: serialize(localSerialAddress),
            });
        }),
        connectSerial: withInvoke<typeof ipcChannelName.net.connectSerial>(({ address, cost }) => {
            return window.ipc[ipcChannelName.net.connectSerial]({
                address: serialize(address),
                cost: serialize(cost),
            });
        }),
        connectUdp: withInvoke<typeof ipcChannelName.net.connectUdp>(({ address, cost }) => {
            return window.ipc[ipcChannelName.net.connectUdp]({
                address: serialize(address),
                cost: serialize(cost),
            });
        }),
        end: withInvoke<typeof ipcChannelName.net.end>(() => {
            return window.ipc[ipcChannelName.net.end]();
        }),
        syncNetState: withInvoke<typeof ipcChannelName.net.syncNetState>(async () => {
            const serialized = await window.ipc[ipcChannelName.net.syncNetState]();
            return StateUpdate.deserialize(serialized);
        }),
        onNetStateUpdate: withListen<typeof ipcChannelName.net.onNetStateUpdate>((callback) => {
            return window.ipc[ipcChannelName.net.onNetStateUpdate]((_, update) => {
                callback(StateUpdate.deserialize(update));
            });
        }),
    },
};
