import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Frame, Protocol, Address } from "@core/net/link";
import { Cost, NodeId } from "@core/net/node";
import { DeserializeResult, InvalidValueError } from "@core/serde";
import { Err, Ok } from "oxide.ts";
import { Source } from "../node/source";

export enum FrameType {
    Hello = 1,
    HelloAck = 2,
    Goodbye = 3,
}

const serializeFrameType = (frameType: FrameType, writer: BufferWriter): void => {
    writer.writeByte(frameType);
};

const deserializeFrameType = (reader: BufferReader): DeserializeResult<FrameType> => {
    const frameType = reader.readByte();
    if (frameType in FrameType) {
        return Ok(frameType);
    } else {
        return Err(new InvalidValueError(`Invalid frame type: ${frameType}`));
    }
};

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

    static deserialize(
        reader: BufferReader,
        type: FrameType.Hello | FrameType.HelloAck,
    ): DeserializeResult<HelloFrame> {
        return Source.deserialize(reader).andThen((source) => {
            return Cost.deserialize(reader).andThen((nodeCost) => {
                return Cost.deserialize(reader).map((linkCost) => {
                    return new HelloFrame({ type, source, nodeCost, linkCost });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        this.source.serialize(writer);
        this.nodeCost.serialize(writer);
        this.linkCost.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.source.serializedLength() + this.nodeCost.serializedLength() + this.linkCost.serializedLength();
    }
}

export class GoodbyeFrame {
    readonly type: FrameType.Goodbye = FrameType.Goodbye;
    senderId: NodeId;

    constructor(opt: { senderId: NodeId }) {
        this.senderId = opt.senderId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<GoodbyeFrame> {
        return NodeId.deserialize(reader).map((senderId) => {
            return new GoodbyeFrame({ senderId });
        });
    }

    serialize(writer: BufferWriter): void {
        serializeFrameType(this.type, writer);
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.senderId.serializedLength();
    }
}

export type NeighborFrame = HelloFrame | GoodbyeFrame;

export const NeighborFrame = {
    deserialize: (reader: BufferReader): DeserializeResult<NeighborFrame> => {
        return deserializeFrameType(reader).andThen((type): DeserializeResult<NeighborFrame> => {
            switch (type) {
                case FrameType.Hello:
                case FrameType.HelloAck:
                    return HelloFrame.deserialize(reader, type);
                case FrameType.Goodbye:
                    return GoodbyeFrame.deserialize(reader);
            }
        });
    },
    serialize: (frame: NeighborFrame, remote: Address): Frame => {
        const buffer = new Uint8Array(frame.serializedLength());
        frame.serialize(new BufferWriter(buffer));
        return {
            protocol: Protocol.RoutingNeighbor,
            remote,
            reader: new BufferReader(buffer),
        };
    },
};
