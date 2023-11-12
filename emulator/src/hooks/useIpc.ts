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

const useInvoke = <T extends IpcInvokeChannelNames>(name: T) => ({
    useInvoke: (opts?: InvokeOptions) => {
        return withSerialized(name, (...args) => {
            window.ipc[name](...(args as any))
                .then(opts?.onSuccess)
                .catch(opts?.onError);
        });
    },
});

const useListen = <T extends IpcListenChannelNames>(name: T) => ({
    useListen: (callback: (...args: IpcSignature[T]["args"]) => void) => {
        useEffect(() => {
            return window.ipc[name](withDeserialized(name, (_, ...args) => callback(...args)));
        }, []);
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
        begin: useInvoke(ipcChannelName.net.begin),
        connectSerial: useInvoke(ipcChannelName.net.connectSerial),
        connectUdp: useInvoke(ipcChannelName.net.connectUdp),
        end: useInvoke(ipcChannelName.net.end),
        onGraphModified: useListen(ipcChannelName.net.onGraphModified),
    },
};
