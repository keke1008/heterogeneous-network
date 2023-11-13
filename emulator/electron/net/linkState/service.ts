import { NetFacade, NodeId } from "@core/net";
import { LinkState, ModifyResult } from "./state";
import { LinkFetcher } from "./linkFetcher";
import { Notification } from "@core/net";
import { EventDispatcher } from "@core/event";

class LinkStateNotifier {
    #state: LinkState = new LinkState();
    #onGraphModified: EventDispatcher<ModifyResult> = new EventDispatcher();

    getLinksNotYetFetchedNodes(): NodeId[] {
        return this.#state.getLinksNotYetFetchedNodes();
    }

    #emitGraphModify(result: ModifyResult[]): void {
        this.#onGraphModified.emit(ModifyResult.merge(...result));
    }

    onGraphModified(onGraphModified: (result: ModifyResult) => void): () => void {
        return this.#onGraphModified.setHandler(onGraphModified);
    }

    handleNotification(notification: Notification) {
        if (notification.target === "node") {
            if (notification.action === "added") {
                const result = notification.nodeIds.map((id) => this.#state.createNode(id));
                this.#emitGraphModify(result);
            } else {
                const result = notification.nodeIds.map((id) => this.#state.removeNode(id));
                this.#emitGraphModify(result);
            }
        } else {
            if (notification.action === "added") {
                const result = notification.linkIds.map((id) => this.#state.createLink(notification.nodeId, id));
                this.#emitGraphModify(result);
            } else {
                const result = notification.linkIds.map((id) => this.#state.removeLink(notification.nodeId, id));
                this.#emitGraphModify(result);
            }
        }
    }
}

export class LinkStateService {
    #state: LinkStateNotifier = new LinkStateNotifier();
    #fetcher: LinkFetcher;
    #onDispose: () => void;

    constructor(net: NetFacade) {
        this.#fetcher = new LinkFetcher(net.rpc());
        this.#onDispose = () => net.notification().onDispose();

        net.notification().subscribe((notification) => {
            this.#state.handleNotification(notification);
        });
        this.#fetcher.onReceive((nodeId, linkIds) => {
            this.#state.handleNotification({ target: "link", action: "added", nodeId, linkIds });
        });
    }

    onGraphModified(onGraphModified: (result: ModifyResult) => void): () => void {
        const fetch = () => {
            for (const nodeId of this.#state.getLinksNotYetFetchedNodes()) {
                this.#fetcher.requestFetch(nodeId);
            }
        };

        const cancel = this.#state.onGraphModified((result) => {
            if (result.addedNodes.length > 0) {
                fetch();
            }
            onGraphModified(result);
        });

        fetch();
        return cancel;
    }

    onDispose(): void {
        this.#onDispose();
    }
}
