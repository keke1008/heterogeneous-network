import { AbortResolve, Destination, ResolveResult, RouteResolver } from "@core/net";
import { RoutingEntry, RoutingTable } from "./table";
import { Matcher } from "./matcher";

export class StaticRouteResolver implements RouteResolver {
    #table = new RoutingTable();

    constructor(table: RoutingTable) {
        this.#table = table;
    }

    async resolve(destination: Destination): Promise<ResolveResult> {
        const gateway = this.#table.resolve(destination);
        return gateway?.resolve() ?? new AbortResolve();
    }

    updateEntry(entry: RoutingEntry): void {
        this.#table.updateEntry(entry);
    }

    deleteEntry(matcher: Matcher): void {
        this.#table.deleteEntry(matcher);
    }

    getEntries(): Readonly<RoutingEntry>[] {
        return this.#table.entries();
    }
}
