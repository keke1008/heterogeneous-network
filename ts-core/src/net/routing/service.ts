import { Result } from "oxide.ts";
import { BufferReader } from "../buffer";
import { Address, LinkSocket } from "../link";
import { NeighborService } from "../neighbor";
import { LocalNodeService, NodeId } from "../node";
import { RoutingBroadcastError, RoutingFrame, RoutingSendError } from "./reactive";

export interface RoutingService {
    resolveGatewayNode(destination: NodeId): Promise<NodeId | undefined>;
    resolveMediaAddresses(destination: NodeId): Promise<Address[] | undefined>;
}

export interface RoutingSocket {
    onReceive(handler: (frame: RoutingFrame) => void): void;
    send(
        destination: NodeId,
        reader: BufferReader,
        ignoreNode?: NodeId,
    ): Promise<Result<void, RoutingSendError | RoutingBroadcastError>>;
}

export interface RoutingSocketConstructor {
    new (args: {
        linkSocket: LinkSocket;
        localNodeService: LocalNodeService;
        neighborService: NeighborService;
        routingService: RoutingService;
        maxFrameIdCacheSize: number;
    }): RoutingSocket;
}
