import { BufferReader, BufferWriter } from "../../buffer";
import { Cost, NodeId } from "../node";
import { FrameId } from "./frameId";

export enum RouteDiscoveryFrameType {
    Request = 1,
    Reply = 2,
}

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

    static deserialize(reader: BufferReader): RouteDiscoveryFrame {
        return new RouteDiscoveryFrame(
            reader.readByte(),
            FrameId.deserialize(reader),
            Cost.deserialize(reader),
            NodeId.deserialize(reader),
            NodeId.deserialize(reader),
            NodeId.deserialize(reader),
        );
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

export const deserializeFrame = (reader: BufferReader): RouteDiscoveryFrame => {
    return RouteDiscoveryFrame.deserialize(reader);
};

export const serializeFrame = (frame: RouteDiscoveryFrame): BufferReader => {
    const buffer = new Uint8Array(frame.serializedLength());
    frame.serialize(new BufferWriter(buffer));
    return new BufferReader(buffer);
};
