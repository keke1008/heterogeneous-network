import { NodeId, UdpAddress } from "@core/net";
import { IpCommand, Route } from "server/src/command/ip";

class RoutingEntryId {
    #id: number;

    constructor(id: number) {
        this.#id = id;
    }

    get id(): number {
        return this.#id;
    }
}

interface RoutingEntry {
    id: RoutingEntryId;
    destinationId: NodeId;
    destinationAddress: UdpAddress;
    gatewayId: NodeId;
    gatewayAddress: UdpAddress;
}

class RoutingTable {
    #ip = new IpCommand();

    async addEntry(entry: { destinationAddress: UdpAddress; gatewayAddress: UdpAddress }): Promise<Route> {
        const id = await this.#ip.routeAdd({
            destination: entry.destinationAddress,
            gateway: entry.gatewayAddress,
        });
        return new RoutingEntryId(id);
    }
}
