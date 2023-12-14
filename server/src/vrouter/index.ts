import { AddressType, NetFacade, NetFacadeBuilder } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { Port } from "../command/nftables";

class VRouterHandle {
    #net: NetFacade = new NetFacadeBuilder().buildWithDefaults();
    #udpHandler: UdpHandler;

    constructor(port: number) {
        this.#udpHandler = new UdpHandler(port);
        this.#net.addHandler(AddressType.Udp, this.#udpHandler);
    }

    onClose(callback: () => void) {
        this.#udpHandler.onClose(callback);
    }

    close() {
        this.#net.dispose();
        this.#udpHandler.close();
    }
}

export class VRouterService {
    #ports: number[] = [10001, 10002, 10003, 10004, 10005];
    #handles = new Map<number, VRouterHandle>();

    async spawn(): Promise<Port | undefined> {
        const port = this.#ports.shift();
        if (port === undefined) {
            return;
        }

        const handle = new VRouterHandle(port);
        handle.onClose(() => this.#ports.push(port));
        this.#handles.set(port, handle);
        return Port.schema.parse(port);
    }

    kill(port: Port): boolean {
        const handle = this.#handles.get(port.toNumber());
        handle?.close();
        return this.#handles.delete(port.toNumber());
    }

    getPorts(): Port[] {
        return Array.from(this.#handles.keys()).map((port) => Port.schema.parse(port));
    }
}
