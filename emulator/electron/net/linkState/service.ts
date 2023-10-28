import { NetFacade, NodeId } from "@core/net";
import { Graph, LinkState } from "./graph";
import { LinkFetcher } from "./linkFetcher";
import { Notification } from "@core/net";

class LinkStateNotifier {
    #state: LinkState;
    #onGraphChanged?: (graph: Graph) => void;

    constructor(selfId: NodeId) {
        this.#state = new LinkState(selfId);
    }

    getLinksNotYetFetchedNodes(): NodeId[] {
        return this.#state.getLinksNotYetFetchedNodes();
    }

    #emitGraphChanged() {
        this.#onGraphChanged?.(this.#state.export());
    }

    onGraphChanged(onGraphChanged: (graph: Graph) => void): void {
        if (this.#onGraphChanged !== undefined) {
            throw new Error("onGraphChanged is already set");
        }
        this.#onGraphChanged = onGraphChanged;
    }

    handleNotification(notification: Notification) {
        if (notification.target === "node") {
            if (notification.action === "added") {
                for (const nodeId of notification.nodeIds) {
                    this.#state.addNodeIfNotExists(nodeId);
                }
            } else {
                for (const nodeId of notification.nodeIds) {
                    this.#state.removeNode(nodeId);
                }
            }
        } else {
            if (notification.action === "added") {
                for (const linkId of notification.linkIds) {
                    this.#state.addLinkIfNotExists(notification.nodeId, linkId);
                }
            } else {
                for (const linkId of notification.linkIds) {
                    this.#state.removeLink(notification.nodeId, linkId);
                }
            }
        }

        this.#emitGraphChanged();
    }
}

export class LinkStateService {
    #state: LinkStateNotifier;
    #fetcher: LinkFetcher;
    #onDispose: () => void;

    constructor(net: NetFacade, selfId: NodeId) {
        this.#state = new LinkStateNotifier(selfId);
        this.#fetcher = new LinkFetcher(net.rpc());
        this.#onDispose = () => net.notification().onDispose();

        net.notification().subscribe((notification) => {
            this.#state.handleNotification(notification);
        });
        this.#fetcher.onReceive((nodeId, linkIds) => {
            this.#state.handleNotification({ target: "link", action: "added", nodeId, linkIds });
        });
    }

    onGraphChanged(onGraphChanged: (graph: Graph) => void): void {
        this.#state.onGraphChanged(onGraphChanged);
        for (const nodeId of this.#state.getLinksNotYetFetchedNodes()) {
            this.#fetcher.requestFetch(nodeId);
        }
    }

    onDispose(): void {
        this.#onDispose();
    }
}
