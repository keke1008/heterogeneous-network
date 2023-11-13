import {
    IpcInvokeChannelNames,
    IpcListenChannelNames,
    IpcSignature,
    ipcChannelName,
    withDeserialized,
    withSerialized,
} from "emulator/electron/ipcChannel";
import { UnionToIntersection } from "@core/types";
import { useEffect } from "react";

export type InvokeOptions = {
    onSuccess?: () => void;
    onError?: () => void;
};

const invokeIpc = <T extends IpcInvokeChannelNames>(name: T) => ({
    useInvoke: (opts?: InvokeOptions) => {
        return withSerialized(name, (...args) => {
            const f = window.ipc[name] as (...args: IpcSignature[T]["serializedArgs"]) => IpcSignature[T]["result"];
            f(...args)
                .then(opts?.onSuccess)
                .catch(opts?.onError);
        });
    },
});

const listinIpc = <T extends IpcListenChannelNames>(name: T) => ({
    useListen: (callback: (...args: IpcSignature[T]["args"]) => void) => {
        useEffect(() => {
            return window.ipc[name](withDeserialized(name, (_, ...args) => callback(...args)));
        }, [callback]);
    },
});

type SingleInvokeHook<T extends string> = T extends IpcInvokeChannelNames
    ? { useInvoke: (opts?: InvokeOptions) => (...args: IpcSignature[T]["args"]) => void }
    : never;

type UnionInvokeHooks<
    Name extends string = IpcInvokeChannelNames,
    Init extends string = "",
> = Name extends `${infer Head}:${infer Rest}`
    ? { [K in Head]: UnionInvokeHooks<Rest, `${Init}${Head}:`> }
    : { [K in Name]: SingleInvokeHook<`${Init}${Name}`> };

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

export const ipc: IpcInvokeHooks & IpcListenHooks = {
    net: {
        begin: invokeIpc(ipcChannelName.net.begin),
        connectSerial: invokeIpc(ipcChannelName.net.connectSerial),
        connectUdp: invokeIpc(ipcChannelName.net.connectUdp),
        end: invokeIpc(ipcChannelName.net.end),
        onGraphModified: listinIpc(ipcChannelName.net.onGraphModified),
    },
};
