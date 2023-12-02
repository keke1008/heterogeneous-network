import { Err, Ok } from "oxide.ts";
import { BufferReader, BufferWriter } from "../../buffer";
import { Cost, NodeId } from "../../node";
import { FrameId } from "@core/net/link";
import { DeserializeResult, InvalidValueError } from "@core/serde";

export enum RouteDiscoveryFrameType {
    Request = 1,
    Reply = 2,
}

const deserializeFrameType = (reader: BufferReader): DeserializeResult<RouteDiscoveryFrameType> => {
    const frameType = reader.readByte();
    if (frameType in RouteDiscoveryFrameType) {
        return Ok(frameType);
    } else {
        return Err(new InvalidValueError(`Invalid frame type: ${frameType}`));
    }
};

export class RouteDiscoveryFrame {
    type: RouteDiscoveryFrameType;
    frameId: FrameId;
    totalCost: Cost;
    sourceId: NodeId;
    targetId: NodeId;
    senderId: NodeId;

    constructor(
        type: RouteDiscoveryFrameType,
        frameId: FrameId,
        totalCost: Cost,
        sourceId: NodeId,
        targetId: NodeId,
        senderId: NodeId,
    ) {
        this.type = type;
        this.frameId = frameId;
        this.totalCost = totalCost;
        this.sourceId = sourceId;
        this.targetId = targetId;
        this.senderId = senderId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RouteDiscoveryFrame> {
        return deserializeFrameType(reader).andThen((frameType) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return NodeId.deserialize(reader).andThen((sourceId) => {
                        return NodeId.deserialize(reader).andThen((targetId) => {
                            return NodeId.deserialize(reader).map((senderId) => {
                                return new RouteDiscoveryFrame(
                                    frameType,
                                    frameId,
                                    totalCost,
                                    sourceId,
                                    targetId,
                                    senderId,
                                );
                            });
                        });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.type);
        this.frameId.serialize(writer);
        this.totalCost.serialize(writer);
        this.sourceId.serialize(writer);
        this.targetId.serialize(writer);
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        const serializers = [this.frameId, this.totalCost, this.sourceId, this.targetId, this.senderId];
        return 1 + serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }
}

export const deserializeFrame = (reader: BufferReader): DeserializeResult<RouteDiscoveryFrame> => {
    return RouteDiscoveryFrame.deserialize(reader);
};

export const serializeFrame = (frame: RouteDiscoveryFrame): BufferReader => {
    const buffer = new Uint8Array(frame.serializedLength());
    frame.serialize(new BufferWriter(buffer));
    return new BufferReader(buffer);
};
