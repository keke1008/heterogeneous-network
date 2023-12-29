import { ArrayDeserializer, ArraySerializer } from "./array";
import { TestReader, TestWriter } from "./helper.test";
import { BooleanSerdeable } from "./primitives";

describe("ArraySerializer", () => {
    it("serializes empty arrays", () => {
        const writer = new TestWriter(0);
        const serializer = new ArraySerializer(new BooleanSerdeable(), []);
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([]));
    });

    it("serializes three booleans", () => {
        const writer = new TestWriter(3);
        const serializer = new ArraySerializer(new BooleanSerdeable(), [true, false, true]);
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([0x01, 0x00, 0x01]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(2);
        const serializer = new ArraySerializer(new BooleanSerdeable(), [true, false, true]);
        expect(serializer.serialize(writer).isOk()).toBe(false);
    });
});

describe("ArrayDeserializer", () => {
    it("deserializes empty arrays", () => {
        const reader = new TestReader([]);
        const deserializer = new ArrayDeserializer(new BooleanSerdeable(), 0);
        expect(deserializer.deserialize(reader).unwrap()).toEqual([]);
    });

    it("deserializes three booleans", () => {
        const data = new Uint8Array([0x01, 0x00, 0x01]);
        const reader = new TestReader(data);
        const deserializer = new ArrayDeserializer(new BooleanSerdeable(), data.length);
        expect(deserializer.deserialize(reader).unwrap()).toEqual([true, false, true]);
    });

    it("fails to deserialize if there is not enough data", () => {
        const reader = new TestReader([0x01, 0x00, 0x01]);
        const deserializer = new ArrayDeserializer(new BooleanSerdeable(), 4);
        expect(deserializer.deserialize(reader).isErr()).toBe(true);
    });
});
