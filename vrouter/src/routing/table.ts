import { Destination } from "@core/net";
import { IGateway } from "./gateway";
import { DefaultMatcher, IMatcher } from "./matcher";

export interface RoutingEntry {
    matcher: IMatcher;
    gateway: IGateway;
}

export class RoutingTable {
    #entries: RoutingEntry[] = [];
    #defaultGateway?: IGateway;

    entries(): Readonly<RoutingEntry>[] {
        if (this.#defaultGateway === undefined) {
            return this.#entries;
        } else {
            const defaultEntry = { matcher: new DefaultMatcher(), gateway: this.#defaultGateway };
            return [...this.#entries, defaultEntry];
        }
    }

    updateEntry(matcher: IMatcher, gateway: IGateway): void {
        if (matcher instanceof DefaultMatcher) {
            this.#defaultGateway = gateway;
            return;
        }

        const index = this.#entries.findIndex((entry) => entry.matcher.equals(matcher));
        if (index === -1) {
            this.#entries.push({ matcher, gateway });
        } else {
            this.#entries[index] = { matcher, gateway };
        }
    }

    deleteEntry(matcher: IMatcher): void {
        if (matcher instanceof DefaultMatcher) {
            this.#defaultGateway = undefined;
            return;
        }

        const index = this.#entries.findIndex((entry) => entry.matcher.equals(matcher));
        if (index !== -1) {
            this.#entries.splice(index, 1);
        }
    }

    resolve(destination: Destination): IGateway | undefined {
        for (const entry of this.entries()) {
            if (entry.matcher.isMatch(destination)) {
                return entry.gateway;
            }
        }
        return this.#defaultGateway;
    }
}
