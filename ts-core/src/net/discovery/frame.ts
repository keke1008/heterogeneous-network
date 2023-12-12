import {
    DeserializeEmpty,
    DeserializeEnum,
    DeserializeResult,
    DeserializeU8,
    DeserializeVariant,
    DeserializeVector,
    SerializeEmpty,
    SerializeEnum,
    SerializeU8,
    SerializeVariant,
    SerializeVector,
} from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Cost, NodeId } from "../node";
import { FrameId } from "./frameId";
import { Address } from "../link";
import { P, match } from "ts-pattern";

export enum DiscoveryFrameType {
    Request,
    Response,
}

const frameTypeSerializer = (frameType: DiscoveryFrameType) => new SerializeEnum(frameType, SerializeU8);
const frameTypeDeserializer = new DeserializeEnum(DiscoveryFrameType, DeserializeU8);

export enum DiscoveryExtraType {
    None,
    ResolveAddresses,
}

const extraTypeSerializer = (extraType: DiscoveryExtraType) => new SerializeEnum(extraType, SerializeU8);
const extraTypeDeserializer = new DeserializeEnum(DiscoveryExtraType, DeserializeU8);

export type Extra = undefined | Address[];

const extraSerializer = (extra: Extra) => {
    return match(extra)
        .with(undefined, () => new SerializeVariant(1, new SerializeEmpty()))
        .with(P.array(), (a) => new SerializeVariant(2, new SerializeVector(a)))
        .exhaustive();
};

const extraDeserializer = new DeserializeVariant(new DeserializeEmpty(), new DeserializeVector(Address));

class DiscoveryCommonFields {
    frameId: FrameId;
    totalCost: Cost;
    sourceId: NodeId;
    destinationId: NodeId;
    senderId: NodeId;

    constructor(args: {
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        destinationId: NodeId;
        senderId: NodeId;
    }) {
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.sourceId = args.sourceId;
        this.destinationId = args.destinationId;
        this.senderId = args.senderId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryCommonFields> {
        return FrameId.deserialize(reader).andThen((frameId) => {
            return Cost.deserialize(reader).andThen((totalCost) => {
                return NodeId.deserialize(reader).andThen((sourceId) => {
                    return NodeId.deserialize(reader).andThen((destinationId) => {
                        return NodeId.deserialize(reader).map((senderId) => {
                            return new DiscoveryCommonFields({
                                frameId,
                                totalCost,
                                sourceId,
                                destinationId,
                                senderId,
                            });
                        });
                    });
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        this.frameId.serialize(writer);
        this.totalCost.serialize(writer);
        this.sourceId.serialize(writer);
        this.destinationId.serialize(writer);
        this.senderId.serialize(writer);
    }

    serializedLength(): number {
        const serializers = [this.frameId, this.totalCost, this.sourceId, this.destinationId, this.senderId];
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }
}

export class DiscoveryRequestFrame {
    type = DiscoveryFrameType.Request as const;
    commonFields: DiscoveryCommonFields;
    extraType: DiscoveryExtraType;

    constructor(args: {
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        destinationId: NodeId;
        senderId: NodeId;
        extraType: DiscoveryExtraType;
    }) {
        this.commonFields = new DiscoveryCommonFields(args);
        this.extraType = args.extraType;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryRequestFrame> {
        return DiscoveryCommonFields.deserialize(reader).andThen((commonFields) => {
            return extraTypeDeserializer.deserialize(reader).map((extraType) => {
                return new DiscoveryRequestFrame({
                    ...commonFields,
                    extraType,
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        frameTypeSerializer(this.type).serialize(writer);
        this.commonFields.serialize(writer);
        extraTypeSerializer(this.extraType).serialize(writer);
    }

    serializedLength(): number {
        const serializers = [frameTypeSerializer(this.type), this.commonFields, extraTypeSerializer(this.extraType)];
        return 1 + serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }
}

export class DiscoveryResponseFrame {
    type = DiscoveryFrameType.Response as const;
    commonFields: DiscoveryCommonFields;
    extra: Extra;

    constructor(args: {
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        destinationId: NodeId;
        senderId: NodeId;
        extra: Extra;
    }) {
        this.commonFields = new DiscoveryCommonFields(args);
        this.extra = args.extra;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryResponseFrame> {
        return DiscoveryCommonFields.deserialize(reader).andThen((commonFields) => {
            return extraDeserializer.deserialize(reader).map((extra) => {
                return new DiscoveryResponseFrame({
                    ...commonFields,
                    extra,
                });
            });
        });
    }

    serialize(writer: BufferWriter): void {
        frameTypeSerializer(this.type).serialize(writer);
        this.commonFields.serialize(writer);
        extraSerializer(this.extra).serialize(writer);
    }

    serializedLength(): number {
        const serializers = [frameTypeSerializer(this.type), this.commonFields, extraSerializer(this.extra)];
        return 1 + serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }
}

export type DiscoveryFrame = DiscoveryRequestFrame | DiscoveryResponseFrame;

export const DiscoveryFrame = {
    deserialize: (reader: BufferReader): DeserializeResult<DiscoveryFrame> => {
        return frameTypeDeserializer.deserialize(reader).andThen<DiscoveryFrame>((frameType) => {
            switch (frameType) {
                case DiscoveryFrameType.Request:
                    return DiscoveryRequestFrame.deserialize(reader);
                case DiscoveryFrameType.Response:
                    return DiscoveryResponseFrame.deserialize(reader);
            }
        });
    },
};
