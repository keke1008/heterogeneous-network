import { Address, AddressClass } from "../link";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeResult } from "@core/serde";

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

    static deserialize(reader: BufferReader): DeserializeResult<NodeId> {
        return Address.deserialize(reader).map((address) => new NodeId(address));
    }

    isLoopback(): boolean {
        return this.#id.isLoopback();
    }

    serialize(writer: BufferWriter): void {
        this.#id.serialize(writer);
    }

    serializedLength(): number {
        return this.#id.serializedLength();
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
