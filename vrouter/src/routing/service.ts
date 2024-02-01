import { Destination, DiscoveryService, NodeId, RoutingService } from "@core/net";
import { RoutingEntry, RoutingTable } from "./table";
import { DiscoveryGateway } from "./gateway";
import { DefaultMatcher, IMatcher } from "./matcher";

export class StaticRoutingService implements RoutingService {
    #discoveryService: DiscoveryService;
    #table = new RoutingTable();

    constructor(args: { discoveryService: DiscoveryService }) {
        this.#discoveryService = args.discoveryService;
        this.#table.updateEntry({ matcher: new DefaultMatcher(), gateway: new DiscoveryGateway() });
    }

    async resolveGatewayNode(destination: Destination): Promise<NodeId | undefined> {
        const gateway = this.#table.resolve(destination);
        if (gateway === undefined) {
            return;
        }

        return gateway.resolve(destination, { discoveryService: this.#discoveryService });
    }

    updateEntry(entry: RoutingEntry): void {
        this.#table.updateEntry(entry);
    }

    deleteEntry(matcher: IMatcher): void {
        this.#table.deleteEntry(matcher);
    }

    getEntries(): Readonly<RoutingEntry>[] {
        return this.#table.entries();
    }
}
