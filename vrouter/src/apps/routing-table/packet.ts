import { ConstantSerdeable, TransformSerdeable, VariantSerdeable, VectorSerdeable } from "@core/serde";
import { Matcher, RoutingEntry } from "@vrouter/routing";

export enum RequestPacketType {
    RequestEntries = 1,
    UpdateEntries = 2,
    DeleteEntries = 3,
}

export class RequestEntries {
    readonly type = RequestPacketType.RequestEntries as const;

    static readonly serdeable = new ConstantSerdeable(new RequestEntries());
}

export class UpdateEntries {
    readonly type = RequestPacketType.UpdateEntries as const;

    entries: RoutingEntry[];

    constructor(entries: RoutingEntry[]) {
        this.entries = entries;
    }

    static readonly serdeable = new TransformSerdeable(
        new VectorSerdeable(RoutingEntry.serdeable),
        (entries) => new UpdateEntries(entries),
        (packet) => packet.entries,
    );
}

export class DeleteEntries {
    readonly type = RequestPacketType.DeleteEntries as const;

    matchers: Matcher[];

    constructor(matchers: Matcher[]) {
        this.matchers = matchers;
    }

    static readonly serdeable = new TransformSerdeable(
        new VectorSerdeable(Matcher.serdeable),
        (matchers) => new DeleteEntries(matchers),
        (packet) => packet.matchers,
    );
}

export type RequestPacket = RequestEntries | UpdateEntries | DeleteEntries;
export const RequestPacket = {
    RequestEntries,
    UpdateEntries,
    DeleteEntries,
    serdeable: new VariantSerdeable(
        [RequestEntries.serdeable, UpdateEntries.serdeable, DeleteEntries.serdeable],
        (req) => req.type,
    ),
};

export enum ResponsePacketType {
    Entries = 1,
}

export class Entries {
    readonly type = ResponsePacketType.Entries as const;

    entries: Readonly<RoutingEntry>[];

    constructor(entries: Readonly<RoutingEntry>[]) {
        this.entries = entries;
    }

    static readonly serdeable = new TransformSerdeable(
        new VectorSerdeable(RoutingEntry.serdeable),
        (entries) => new Entries(entries),
        (packet) => packet.entries,
    );
}

export type ResponsePacket = Entries;
export const ResponsePacket = {
    Entries,
    serdeable: new VariantSerdeable([Entries.serdeable], (res) => res.type),
};
