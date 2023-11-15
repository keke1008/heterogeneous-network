import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Frame, Protocol, Address, AddressError } from "@core/net/link";
import { Cost, NodeId } from "../node";
import { Result } from "oxide.ts";

export enum FrameType {
    Hello = 1,
    HelloAck = 2,
    Goodbye = 3,
}

const frameTypeToNumber = (frameType: FrameType): number => {
    if (!(frameType in FrameType)) {
        throw new Error(`Unknown frame type: ${frameType}`);
    }
    return frameType;
};

const numberToFrameType = (number: number): FrameType => {
    if (!(number in FrameType)) {
        throw new Error(`Unknown frame type number: ${number}`);
    }
    return number;
};

export class HelloFrame {
    readonly type: FrameType.Hello | FrameType.HelloAck;
    senderId: NodeId;
    edgeCost: Cost;

    constructor(opt: { type: FrameType.Hello | FrameType.HelloAck; senderId: NodeId; edgeCost: Cost }) {
        this.type = opt.type;
        this.senderId = opt.senderId;
        this.edgeCost = opt.edgeCost;
    }

    static deserialize(
        reader: BufferReader,
        type: FrameType.Hello | FrameType.HelloAck,
    ): Result<HelloFrame, AddressError> {
        return NodeId.deserialize(reader).andThen((senderId) => {
            return Cost.deserialize(reader).map((edgeCost) => {
                return new HelloFrame({ type, senderId, edgeCost });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(frameTypeToNumber(this.type));
        this.senderId.serialize(writer);
        this.edgeCost.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.senderId.serializedLength() + this.edgeCost.serializedLength();
    }
}

export class GoodbyeFrame {
    readonly type: FrameType.Goodbye = FrameType.Goodbye;
    senderId: NodeId;

    constructor(opt: { senderId: NodeId }) {
        this.senderId = opt.senderId;
    }

    static deserialize(reader: BufferReader): Result<GoodbyeFrame, AddressError> {
        return NodeId.deserialize(reader).map((senderId) => {
            return new GoodbyeFrame({ senderId });
        });
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(frameTypeToNumber(this.type));
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        return 1 + this.senderId.serializedLength();
    }
}

export type NeighborFrame = HelloFrame | GoodbyeFrame;

export const deserializeFrame = (reader: BufferReader): Result<NeighborFrame, AddressError> => {
    const type = numberToFrameType(reader.readByte());
    switch (type) {
        case FrameType.Hello:
        case FrameType.HelloAck:
            return HelloFrame.deserialize(reader, type);
        case FrameType.Goodbye:
            return GoodbyeFrame.deserialize(reader);
    }
};

export const serializeFrame = (frame: NeighborFrame, peer: Address): Frame => {
    const buffer = new Uint8Array(frame.serializedLength());
    frame.serialize(new BufferWriter(buffer));
    return {
        protocol: Protocol.RoutingNeighbor,
        remote: peer,
        reader: new BufferReader(buffer),
    };
};
