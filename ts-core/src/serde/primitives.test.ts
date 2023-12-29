import { TestReader, TestWriter } from "./helper.test";
import {
    BooleanDeserializer,
    BooleanSerializer,
    Uint16Deserializer,
    Uint16Serializer,
    Uint32Deserializer,
    Uint32Serializer,
    Uint8Deserializer,
    Uint8Serializer,
} from "./primitives";

describe("BooleanSerializer", () => {
    it("serializes true", () => {
        const writer = new TestWriter(1);
        expect(new BooleanSerializer(true).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([0x01]));
    });

    it("serializes false", () => {
        const writer = new TestWriter(1);
        expect(new BooleanSerializer(false).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([0x00]));
    });
});

describe("BooleanDeserializer", () => {
    it("deserializes true", () => {
        const reader = new TestReader(new Uint8Array([0x01]));
        expect(new BooleanDeserializer().deserialize(reader).unwrap()).toEqual(true);
    });

    it("deserializes false", () => {
        const reader = new TestReader(new Uint8Array([0x00]));
        expect(new BooleanDeserializer().deserialize(reader).unwrap()).toEqual(false);
    });

    it("fails to deserialize other values", () => {
        const reader = new TestReader(new Uint8Array([0x02]));
        expect(new BooleanDeserializer().deserialize(reader).isOk()).toBe(false);
    });
});

describe("Uint8Serializer", () => {
    it.each([0, 1, 0xff])("serializes %p", (n) => {
        const writer = new TestWriter(1);
        expect(new Uint8Serializer(n).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([n]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(0);
        expect(new Uint8Serializer(0).serialize(writer).isOk()).toBe(false);
    });
});

describe("Uint8Deserializer", () => {
    it.each([0, 1, 0xff])("deserializes %p", (n) => {
        const reader = new TestReader(new Uint8Array([n]));
        expect(new Uint8Deserializer().deserialize(reader).isOk()).toBe(true);
    });

    it("fails to deserialize if there is not enough bytes", () => {
        const reader = new TestReader(new Uint8Array([]));
        expect(new Uint8Deserializer().deserialize(reader).isOk()).toBe(false);
    });
});

describe("Uint16Serializer", () => {
    it.each([0, 1, 0xffff])("serializes %p", (n) => {
        const writer = new TestWriter(2);
        expect(new Uint16Serializer(n).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([n & 0xff, (n >> 8) & 0xff]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(1);
        expect(new Uint16Serializer(0).serialize(writer).isOk()).toBe(false);
    });
});

describe("Uint16Deserializer", () => {
    it.each([0, 1, 0xffff])("deserializes %p", (n) => {
        const reader = new TestReader(new Uint8Array([n & 0xff, (n >> 8) & 0xff]));
        expect(new Uint16Deserializer().deserialize(reader).isOk()).toBe(true);
    });

    it("fails to deserialize if there is not enough bytes", () => {
        const reader = new TestReader(new Uint8Array([]));
        expect(new Uint16Deserializer().deserialize(reader).isOk()).toBe(false);
    });
});

describe("Uint32Serializer", () => {
    it.each([0, 1, 0xffffffff])("serializes %p", (n) => {
        const writer = new TestWriter(4);
        expect(new Uint32Serializer(n).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([n & 0xff, (n >> 8) & 0xff, (n >> 16) & 0xff, (n >> 24) & 0xff]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(3);
        expect(new Uint32Serializer(0).serialize(writer).isOk()).toBe(false);
    });
});

describe("Uint32Deserializer", () => {
    it.each([0, 1, 0xffffffff])("deserializes %p", (n) => {
        const reader = new TestReader(new Uint8Array([n & 0xff, (n >> 8) & 0xff, (n >> 16) & 0xff, (n >> 24) & 0xff]));
        expect(new Uint32Deserializer().deserialize(reader).isOk()).toBe(true);
    });

    it("fails to deserialize if there is not enough bytes", () => {
        const reader = new TestReader(new Uint8Array([]));
        expect(new Uint32Deserializer().deserialize(reader).isOk()).toBe(false);
    });
});
