import { Cost, NodeId } from "@core/net/node";
import { ObjectSerdeable, TransformSerdeable, Uint8Serdeable } from "@core/serde";
import { BitflagsSerdeable } from "@core/serde/bitflags";

export enum NeighborControlFlags {
    Empty = 0,
    KeepAlive = 1,
}

export class NeighborControlFrame {
    flags: NeighborControlFlags;
    sourceNodeId: NodeId;
    linkCost: Cost;

    constructor(opt: { flags: NeighborControlFlags; sourceNodeId: NodeId; linkCost: Cost }) {
        this.flags = opt.flags;
        this.sourceNodeId = opt.sourceNodeId;
        this.linkCost = opt.linkCost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            flags: new BitflagsSerdeable<NeighborControlFlags>(NeighborControlFlags, new Uint8Serdeable()),
            sourceNodeId: NodeId.serdeable,
            linkCost: Cost.serdeable,
        }),
        (obj) => new NeighborControlFrame(obj),
        (frame) => frame,
    );

    shouldReplyImmediately(): boolean {
        return (this.flags & NeighborControlFlags.KeepAlive) === 0;
    }
}
