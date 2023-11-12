import { Address, AddressClass, AddressError } from "../link";
import { BufferReader, BufferWriter } from "../buffer";
import { Ok, Result } from "oxide.ts";

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

    static deserialize(reader: BufferReader): Result<NodeId, AddressError> {
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

export class Cost {
    #cost: number;

    constructor(cost: number) {
        if (cost < 0 || cost > 0xffff) {
            throw new Error(`Cost must be between 0 and 65535, but was ${cost}`);
        }
        this.#cost = cost;
    }

    get(): number {
        return this.#cost;
    }

    equals(other: Cost): boolean {
        return this.#cost === other.#cost;
    }

    add(other: Cost): Cost {
        return new Cost(this.#cost + other.#cost);
    }

    lessThan(other: Cost): boolean {
        return this.#cost < other.#cost;
    }

    static deserialize(reader: BufferReader): Result<Cost, never> {
        return Ok(new Cost(reader.readUint16()));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.#cost & 0xff);
        writer.writeByte(this.#cost & 0xff00);
    }

    serializedLength(): number {
        return 2;
    }
}
