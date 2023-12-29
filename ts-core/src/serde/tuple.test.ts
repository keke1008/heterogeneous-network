import { TestReader, TestWriter } from "./helper.test";
import { Uint8Serializer, BooleanSerializer, BooleanDeserializer, Uint8Deserializer } from "./primitives";
import { TupleDeserializer, TupleSerializer } from "./tuple";

describe("TupleSerializer", () => {
    it("serializes tuple", () => {
        const writer = new TestWriter(3);
        const serializer = new TupleSerializer([
            new Uint8Serializer(2),
            new BooleanSerializer(true),
            new Uint8Serializer(3),
        ]);
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([2, 1, 3]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(2);
        const serializer = new TupleSerializer([
            new Uint8Serializer(2),
            new BooleanSerializer(true),
            new Uint8Serializer(3),
        ]);
        expect(serializer.serialize(writer).isOk()).toBe(false);
    });
});

describe("TupleDeserializer", () => {
    it("deserializes tuple", () => {
        const reader = new TestReader([2, 1, 3]);
        const deserializer = new TupleDeserializer([
            new Uint8Deserializer(),
            new BooleanDeserializer(),
            new Uint8Deserializer(),
        ]);
        expect(deserializer.deserialize(reader).unwrap()).toEqual([2, true, 3]);
    });

    it("fails to deserialize if there is not enough data", () => {
        const reader = new TestReader([]);
        const deserializer = new TupleDeserializer([
            new Uint8Deserializer(),
            new BooleanDeserializer(),
            new Uint8Deserializer(),
        ]);
        expect(deserializer.deserialize(reader).isErr()).toBe(true);
    });
});
