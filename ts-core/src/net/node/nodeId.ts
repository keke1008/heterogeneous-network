import { Address, AddressClass, AddressType } from "../link";
import { BytesSerdeable, ManualVariantSerdeable, TransformSerdeable } from "@core/serde";

export enum NodeIdType {
    // 特別な意味を持つID
    Broadcast = 0xff,
    Loopback = 0x7f,

    // Address互換のID
    Serial = 0x01,
    UHF = 0x02,
    IPv4 = 0x03,
    WebSocket = 0x04,
}

const bodyBytesSizeOf = (type: NodeIdType): number => {
    switch (type) {
        case NodeIdType.Broadcast:
            return 0;
        case NodeIdType.Loopback:
        case NodeIdType.Serial:
        case NodeIdType.UHF:
        case NodeIdType.IPv4:
        case NodeIdType.WebSocket: {
            const address = Address.table[type as unknown as AddressType];
            return address.bodyBytesSize();
        }
    }
};

const addressTypeToNodeIdType = (type: AddressType): NodeIdType => {
    return type as unknown as NodeIdType;
};

export class NodeId {
    #type: NodeIdType;
    #body: Uint8Array;

    private constructor(type: NodeIdType, body: Uint8Array) {
        if (body.length !== bodyBytesSizeOf(type)) {
            throw new Error(`NodeId: invalid body length: ${body.length}`);
        }
        this.#type = type;
        this.#body = body;
    }

    static broadcast(): NodeId {
        return new NodeId(NodeIdType.Broadcast, new Uint8Array());
    }

    static loopback(): NodeId {
        return new NodeId(NodeIdType.Loopback, new Uint8Array());
    }

    static fromAddress(address: AddressClass): NodeId {
        return new NodeId(addressTypeToNodeIdType(address.type), address.body());
    }

    static readonly serdeable = new ManualVariantSerdeable<NodeId>(
        (nodeId) => nodeId.#type,
        (type) =>
            new TransformSerdeable(
                new BytesSerdeable(bodyBytesSizeOf(type as NodeIdType)),
                (body) => new NodeId(type as NodeIdType, body),
                (nodeId) => nodeId.#body,
            ),
    );

    isBroadcast(): boolean {
        return this.#type === NodeIdType.Broadcast;
    }

    isLoopback(): boolean {
        return this.#type === NodeIdType.Loopback;
    }

    equals(other: NodeId): boolean {
        return this.#type === other.#type && this.#body === other.#body;
    }

    toString(): string {
        return `NodeId(${this.#type}, ${this.#body})`;
    }

    display(): string {
        return `NodeId(${this.#type}, [${this.#body}])`;
    }
}
