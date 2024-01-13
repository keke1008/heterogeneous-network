import { NodeId } from "@core/net/node";

export class ReceivedNeighborFrame {
    sender: NodeId;
    payload: Uint8Array;

    constructor(args: { sender: NodeId; payload: Uint8Array }) {
        this.sender = args.sender;
        this.payload = args.payload;
    }
}
