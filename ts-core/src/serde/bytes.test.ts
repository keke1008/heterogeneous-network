import { FixedLengthBytesDeserializer, FixedBytesSerializer } from "./bytes";
import { TestReader, TestWriter } from "./helper.test";

describe("BytesSerializer", () => {
    it.each([[], [0], [0, 1, 2]].map((a) => new Uint8Array(a)))(`serializes %p`, (data) => {
        const writer = new TestWriter(data.length);
        const serializer = new FixedBytesSerializer(data);
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(data);
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(2);
        const serializer = new FixedBytesSerializer(new Uint8Array([0, 1, 2]));
        expect(serializer.serialize(writer).isOk()).toBe(false);
    });
});

describe("BytesDeserializer", () => {
    it.each([[], [0], [0, 1, 2]].map((a) => new Uint8Array(a)))(`deserializes %p`, (data) => {
        const reader = new TestReader(data);
        const deserializer = new FixedLengthBytesDeserializer(data.length);
        expect(deserializer.deserialize(reader).unwrap()).toEqual(data);
    });

    it("fails to deserialize if there is not enough data", () => {
        const reader = new TestReader(new Uint8Array([0, 1, 2]));
        const deserializer = new FixedLengthBytesDeserializer(4);
        expect(deserializer.deserialize(reader).isErr()).toBe(true);
    });
});
