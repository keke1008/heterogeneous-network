import { Address, AddressClass } from "../link";
import { BufferReader, BufferWriter } from "../buffer";
import { DeserializeResult } from "@core/serde";

export class NodeId {
    #id: Address;

    constructor(id: Address) {
        this.#id = id;
    }

    static broadcast(): NodeId {
        return new NodeId(Address.broadcast());
    }

    static fromAddress(address: AddressClass): NodeId {
        return new NodeId(new Address(address));
    }

    static deserialize(reader: BufferReader): DeserializeResult<NodeId> {
        return Address.deserialize(reader).map((address) => new NodeId(address));
    }

    isBroadcast(): boolean {
        return this.#id.isBroadcast();
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
}
