import { Err, Ok } from "oxide.ts";
import { BufferReader, BufferWriter } from "../../buffer";
import { Cost, LocalNodeService, NodeId } from "../../node";
import { FrameId, FrameIdCache } from "../frameId";
import {
    DeserializeEmpty,
    DeserializeEnum,
    DeserializeResult,
    DeserializeU8,
    DeserializeVariant,
    DeserializeVector,
    InvalidValueError,
    SerializeEmpty,
    SerializeEnum,
    SerializeU8,
    SerializeVariant,
    SerializeVector,
} from "@core/serde";
import { Address, LinkService } from "@core/net/link";
import { P, match } from "ts-pattern";

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

export enum ExtraSpecifier {
    None = 1,
    MediaAddress = 2,
}
const extraSpecifierDeserializer = new DeserializeEnum(ExtraSpecifier, DeserializeU8);

export type Extra = ExtraSpecifier | undefined | Address[];

const extraSerializer = (extra: Extra) => {
    return match(extra)
        .with(undefined, () => new SerializeVariant(2, new SerializeEmpty()))
        .with(P.array(), (a) => new SerializeVariant(3, new SerializeVector(a)))
        .otherwise((spec) => new SerializeVariant(1, new SerializeEnum(spec, SerializeU8)));
};

const extraDeserializer = new DeserializeVariant(
    extraSpecifierDeserializer,
    new DeserializeEmpty(),
    new DeserializeVector(Address),
);

export class RouteDiscoveryFrame {
    type: RouteDiscoveryFrameType;
    frameId: FrameId;
    totalCost: Cost;
    sourceId: NodeId;
    targetId: NodeId;
    senderId: NodeId;
    extra: Extra;

    constructor(args: {
        type: RouteDiscoveryFrameType;
        frameId: FrameId;
        totalCost: Cost;
        sourceId: NodeId;
        targetId: NodeId;
        senderId: NodeId;
        extra: Extra;
    }) {
        this.type = args.type;
        this.frameId = args.frameId;
        this.totalCost = args.totalCost;
        this.sourceId = args.sourceId;
        this.targetId = args.targetId;
        this.senderId = args.senderId;
        this.extra = args.extra;
    }

    static async request(
        frameIdCache: FrameIdCache,
        localNodeService: LocalNodeService,
        targetId: NodeId,
        extra: ExtraSpecifier = ExtraSpecifier.None,
    ): Promise<RouteDiscoveryFrame> {
        const localId = await localNodeService.getId();
        return new RouteDiscoveryFrame({
            type: RouteDiscoveryFrameType.Request,
            frameId: frameIdCache.generate(),
            totalCost: new Cost(0),
            sourceId: localId,
            targetId,
            senderId: localId,
            extra,
        });
    }

    static deserialize(reader: BufferReader): DeserializeResult<RouteDiscoveryFrame> {
        return deserializeFrameType(reader).andThen((type) => {
            return FrameId.deserialize(reader).andThen((frameId) => {
                return Cost.deserialize(reader).andThen((totalCost) => {
                    return NodeId.deserialize(reader).andThen((sourceId) => {
                        return NodeId.deserialize(reader).andThen((targetId) => {
                            return NodeId.deserialize(reader).andThen((senderId) => {
                                return extraDeserializer.deserialize(reader).map((extra) => {
                                    return new RouteDiscoveryFrame({
                                        type,
                                        frameId,
                                        totalCost,
                                        sourceId,
                                        targetId,
                                        senderId,
                                        extra,
                                    });
                                });
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
        extraSerializer(this.extra).serialize(writer);
    }

    serializedLength(): number {
        const serializers = [
            this.frameId,
            this.totalCost,
            this.sourceId,
            this.targetId,
            this.senderId,
            extraSerializer(this.extra),
        ];
        return 1 + serializers.reduce((acc, s) => acc + s.serializedLength(), 0);
    }

    async repeat(localNodeService: LocalNodeService, edgeCost: Cost): Promise<RouteDiscoveryFrame> {
        const { id: localId, cost: localCost } = await localNodeService.getInfo();
        return new RouteDiscoveryFrame({
            type: this.type,
            frameId: this.frameId,
            totalCost: this.totalCost.add(edgeCost).add(localCost),
            sourceId: this.sourceId,
            targetId: this.targetId,
            senderId: localId,
            extra: this.extra,
        });
    }

    async reply(
        linkService: LinkService,
        localNodeService: LocalNodeService,
        frameIdCache: FrameIdCache,
    ): Promise<RouteDiscoveryFrame> {
        const localId = await localNodeService.getId();
        const extra = this.extra === ExtraSpecifier.MediaAddress ? linkService.getLocalAddresses() : undefined;
        return new RouteDiscoveryFrame({
            type: RouteDiscoveryFrameType.Reply,
            frameId: frameIdCache.generate(),
            totalCost: new Cost(0),
            sourceId: localId,
            targetId: this.sourceId,
            senderId: localId,
            extra,
        });
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
