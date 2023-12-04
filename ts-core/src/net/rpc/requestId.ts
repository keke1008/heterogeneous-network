import { DeserializeResult } from "@core/serde";
import { BufferReader, BufferWriter } from "../buffer";
import { Ok } from "oxide.ts";

export class RequestId {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    get(): number {
        return this.#value;
    }

    equals(other: RequestId): boolean {
        return this.#value === other.#value;
    }

    static deserialize(reader: BufferReader): DeserializeResult<RequestId> {
        return Ok(new RequestId(reader.readUint16()));
    }

    serialize(writer: BufferWriter): void {
        writer.writeUint16(this.#value);
    }

    serializedLength(): number {
        return 2;
    }
}

export class IncrementalRequestIdGenerator {
    #nextId = 0;

    generate(): RequestId {
        return new RequestId(this.#nextId++);
    }
}


