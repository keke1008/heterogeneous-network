import { Address, AddressClass } from "../link";
import { TransformSerdeable } from "@core/serde";

export class NodeId {
    #id: Address;

    constructor(id: Address) {
        this.#id = id;
    }

    static loopback(): NodeId {
        return new NodeId(Address.loopback());
    }

    static fromAddress(address: AddressClass): NodeId {
        return new NodeId(new Address(address));
    }

    static readonly serdeable = new TransformSerdeable(
        Address.serdeable,
        (address) => new NodeId(address),
        (nodeId) => nodeId.#id,
    );

    isLoopback(): boolean {
        return this.#id.isLoopback();
    }

    equals(other: NodeId): boolean {
        return this.#id.equals(other.#id);
    }

    toString(): string {
        return this.#id.toString();
    }

    display(): string {
        return `NodeId(${this.#id.display()})`;
    }
}
