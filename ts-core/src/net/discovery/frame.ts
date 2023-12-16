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
    Request = 1,
    Response = 2,
}

const frameTypeSerializer = (frameType: DiscoveryFrameType) => new SerializeEnum(frameType, SerializeU8);
const frameTypeDeserializer = new DeserializeEnum(DiscoveryFrameType, DeserializeU8);

export enum DiscoveryExtraType {
    None = 1,
    ResolveAddresses = 2,
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

    private constructor(args: {
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

    static request(args: { frameId: FrameId; destinationId: NodeId; localId: NodeId }) {
        return new DiscoveryCommonFields({
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: args.localId,
            destinationId: args.destinationId,
            senderId: args.localId,
        });
    }

    repeat(args: { localId: NodeId; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryCommonFields({
            frameId: this.frameId,
            totalCost: this.totalCost.add(args.sourceLinkCost).add(args.localCost),
            sourceId: this.sourceId,
            destinationId: this.destinationId,
            senderId: args.localId,
        });
    }

    reply(args: { localId: NodeId; frameId: FrameId }) {
        return new DiscoveryCommonFields({
            frameId: args.frameId,
            totalCost: new Cost(0),
            sourceId: this.destinationId,
            destinationId: this.sourceId,
            senderId: args.localId,
        });
    }
}

export class DiscoveryRequestFrame {
    type = DiscoveryFrameType.Request as const;
    commonFields: DiscoveryCommonFields;
    extraType: DiscoveryExtraType;

    private constructor(args: { commonFields: DiscoveryCommonFields; extraType: DiscoveryExtraType }) {
        this.commonFields = args.commonFields;
        this.extraType = args.extraType;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryRequestFrame> {
        return DiscoveryCommonFields.deserialize(reader).andThen((commonFields) => {
            return extraTypeDeserializer.deserialize(reader).map((extraType) => {
                return new DiscoveryRequestFrame({ commonFields, extraType });
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
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static request(args: { frameId: FrameId; destinationId: NodeId; localId: NodeId; extraType: DiscoveryExtraType }) {
        return new DiscoveryRequestFrame({
            commonFields: DiscoveryCommonFields.request({
                frameId: args.frameId,
                destinationId: args.destinationId,
                localId: args.localId,
            }),
            extraType: args.extraType,
        });
    }

    repeat(args: { localId: NodeId; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryRequestFrame({
            commonFields: this.commonFields.repeat({
                localId: args.localId,
                sourceLinkCost: args.sourceLinkCost,
                localCost: args.localCost,
            }),
            extraType: this.extraType,
        });
    }
}

export class DiscoveryResponseFrame {
    type = DiscoveryFrameType.Response as const;
    commonFields: DiscoveryCommonFields;
    extra: Extra;

    private constructor(args: { commonFields: DiscoveryCommonFields; extra: Extra }) {
        this.commonFields = args.commonFields;
        this.extra = args.extra;
    }

    static deserialize(reader: BufferReader): DeserializeResult<DiscoveryResponseFrame> {
        return DiscoveryCommonFields.deserialize(reader).andThen((commonFields) => {
            return extraDeserializer.deserialize(reader).map((extra) => {
                return new DiscoveryResponseFrame({ commonFields, extra });
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
        return serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    static reply(request: DiscoveryRequestFrame, args: { localId: NodeId; frameId: FrameId; extra: Extra }) {
        return new DiscoveryResponseFrame({
            commonFields: request.commonFields.reply({ localId: args.localId, frameId: args.frameId }),
            extra: args.extra,
        });
    }

    repeat(args: { localId: NodeId; sourceLinkCost: Cost; localCost: Cost }) {
        return new DiscoveryResponseFrame({
            commonFields: this.commonFields.repeat({
                localId: args.localId,
                sourceLinkCost: args.sourceLinkCost,
                localCost: args.localCost,
            }),
            extra: this.extra,
        });
    }
}

export type DiscoveryFrame = DiscoveryRequestFrame | DiscoveryResponseFrame;

export const DiscoveryFrame = {
    deserialize: (reader: BufferReader): DeserializeResult<DiscoveryFrame> => {
        return frameTypeDeserializer.deserialize(reader).andThen<DiscoveryFrame>((frameType) => {
            return match(frameType)
                .with(DiscoveryFrameType.Request, () => DiscoveryRequestFrame.deserialize(reader))
                .with(DiscoveryFrameType.Response, () => DiscoveryResponseFrame.deserialize(reader))
                .run();
        });
    },
};
