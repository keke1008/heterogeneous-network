import { Cost, NodeId, Source } from "@core/net/node";
import { TransformSerdeable, TupleSerdeable, VariantSerdeable } from "@core/serde";

export enum FrameType {
    Hello = 1,
    HelloAck = 2,
    Goodbye = 3,
}

export class HelloFrame {
    readonly type: FrameType.Hello | FrameType.HelloAck;
    source: Source;
    nodeCost: Cost;
    linkCost: Cost;

    constructor(opt: { type: FrameType.Hello | FrameType.HelloAck; source: Source; nodeCost: Cost; linkCost: Cost }) {
        this.type = opt.type;
        this.source = opt.source;
        this.nodeCost = opt.nodeCost;
        this.linkCost = opt.linkCost;
    }

    static readonly serdeable = (type: FrameType.Hello | FrameType.HelloAck) =>
        new TransformSerdeable(
            new TupleSerdeable([Source.serdeable, Cost.serdeable, Cost.serdeable] as const),
            ([source, nodeCost, linkCost]) => new HelloFrame({ type, source, nodeCost, linkCost }),
            (frame) => [frame.source, frame.nodeCost, frame.linkCost] as const,
        );
}

export class GoodbyeFrame {
    readonly type: FrameType.Goodbye = FrameType.Goodbye;
    senderId: NodeId;

    constructor(opt: { senderId: NodeId }) {
        this.senderId = opt.senderId;
    }

    static readonly serdeable = new TransformSerdeable(
        NodeId.serdeable,
        (senderId) => new GoodbyeFrame({ senderId }),
        (frame) => frame.senderId,
    );
}

export type NeighborFrame = HelloFrame | GoodbyeFrame;

export const NeighborFrame = {
    serdeable: new VariantSerdeable(
        [HelloFrame.serdeable(FrameType.Hello), HelloFrame.serdeable(FrameType.HelloAck), GoodbyeFrame.serdeable],
        (frame) => frame.type,
    ),
};
