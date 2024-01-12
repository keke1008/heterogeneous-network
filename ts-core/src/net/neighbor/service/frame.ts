import { Cost, Source } from "@core/net/node";
import { ObjectSerdeable, TransformSerdeable, Uint8Serdeable } from "@core/serde";
import { BitflagsSerdeable } from "@core/serde/bitflags";

export enum NeighborControlFlags {
    Empty = 0,
    KeepAlive = 1,
}

export class NeighborControlFrame {
    flags: NeighborControlFlags;
    source: Source;
    sourceCost: Cost;
    linkCost: Cost;

    constructor(opt: { flags: NeighborControlFlags; source: Source; sourceCost: Cost; linkCost: Cost }) {
        this.flags = opt.flags;
        this.source = opt.source;
        this.sourceCost = opt.sourceCost;
        this.linkCost = opt.linkCost;
    }

    static readonly serdeable = new TransformSerdeable(
        new ObjectSerdeable({
            flags: new BitflagsSerdeable<NeighborControlFlags>(NeighborControlFlags, new Uint8Serdeable()),
            source: Source.serdeable,
            sourceCost: Cost.serdeable,
            linkCost: Cost.serdeable,
        }),
        (obj) => new NeighborControlFrame(obj),
        (frame) => frame,
    );

    shouldReplyImmediately(): boolean {
        return (this.flags & NeighborControlFlags.KeepAlive) === 0;
    }
}
