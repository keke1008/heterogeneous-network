import { AddressType, NetFacade, NetFacadeBuilder, NodeId, UdpAddress } from "@core/net";
import { UdpHandler, getLocalIpV4Addresses } from "@media/udp-node";
import { DefaultMatcher, DiscoveryGateway, StaticRouteResolver } from "../routing";
import * as z from "zod";
import { RoutingTableServer } from "@vrouter/apps/routing-table";
import { RoutingTable } from "@vrouter/routing/table";

export class VRouterPort {
    #port: number;

    private constructor(args: { port: number }) {
        this.#port = args.port;
    }

    toString(): string {
        return this.#port.toString();
    }

    equals(other: VRouterPort): boolean {
        return this.#port === other.#port;
    }

    static readonly schema = z.coerce
        .number()
        .min(0)
        .max(65535)
        .transform((value) => new VRouterPort({ port: value }));

    toNumber(): number {
        return this.#port;
    }
}

class VRouterHandle {
    #net: NetFacade;
    #udpHandler: UdpHandler;
    #table = new RoutingTable();
    #routingTableAppServer = new RoutingTableServer({ table: this.#table });

    constructor(port: number) {
        this.#table.updateEntry({ matcher: new DefaultMatcher(), gateway: new DiscoveryGateway() });

        this.#net = new NetFacadeBuilder().buildWithDefaults();
        this.#net.discovery().injectRouteResolver(new StaticRouteResolver(this.#table));

        this.#udpHandler = new UdpHandler(port);
        this.#net.addHandler(AddressType.Udp, this.#udpHandler);

        const ipAddress = getLocalIpV4Addresses()[0];
        const address = UdpAddress.schema.parse([ipAddress, port]);
        this.#net.localNode().tryInitialize(NodeId.fromAddress(address));

        this.#routingTableAppServer.start({ streamService: this.#net.stream() }).unwrap();
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

    async spawn(): Promise<VRouterPort | undefined> {
        const port = this.#ports.shift();
        if (port === undefined) {
            return;
        }

        const handle = new VRouterHandle(port);
        handle.onClose(() => this.#ports.push(port));
        this.#handles.set(port, handle);
        return VRouterPort.schema.parse(port);
    }

    kill(port: VRouterPort): boolean {
        const handle = this.#handles.get(port.toNumber());
        handle?.close();
        return this.#handles.delete(port.toNumber());
    }

    getPorts(): VRouterPort[] {
        return Array.from(this.#handles.keys()).map((port) => VRouterPort.schema.parse(port));
    }
}
