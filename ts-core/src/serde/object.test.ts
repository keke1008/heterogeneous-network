import { TestReader, TestWriter } from "./helper.test";
import { ObjectDeserializer, ObjectSerdeable, ObjectSerializer } from "./object";
import {
    BooleanDeserializer,
    BooleanSerdeable,
    BooleanSerializer,
    Uint8Deserializer,
    Uint8Serdeable,
    Uint8Serializer,
} from "./primitives";

describe("ObjectSerializer", () => {
    it("serializes empty objects", () => {
        const writer = new TestWriter(0);
        const serializer = new ObjectSerializer({});
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([]));
    });

    it("serializes objects with one field", () => {
        const writer = new TestWriter(1);
        const serializer = new ObjectSerializer({ a: new Uint8Serializer(1) });
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([1]));
    });

    it("serializes objects with many fields", () => {
        const writer = new TestWriter(3);
        const serializer = new ObjectSerializer({
            a: new Uint8Serializer(2),
            b: new BooleanSerializer(true),
            c: new Uint8Serializer(3),
        });
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([2, 1, 3]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(2);
        const serializer = new ObjectSerializer({
            a: new Uint8Serializer(2),
            b: new BooleanSerializer(true),
            c: new Uint8Serializer(3),
        });
        expect(serializer.serialize(writer).isOk()).toBe(false);
    });
});

describe("ObjectDeserializer", () => {
    it("deserializes empty objects", () => {
        const reader = new TestReader([]);
        const deserializer = new ObjectDeserializer({});
        expect(deserializer.deserialize(reader).unwrap()).toEqual({});
    });

    it("deserializes objects with one field", () => {
        const reader = new TestReader([1]);
        const deserializer = new ObjectDeserializer({ a: new Uint8Deserializer() });
        expect(deserializer.deserialize(reader).unwrap()).toEqual({ a: 1 });
    });

    it("deserializes objects with many fields", () => {
        const reader = new TestReader([2, 1, 3]);
        const deserializer = new ObjectDeserializer({
            a: new Uint8Deserializer(),
            b: new BooleanDeserializer(),
            c: new Uint8Deserializer(),
        });
        expect(deserializer.deserialize(reader).unwrap()).toEqual({ a: 2, b: true, c: 3 });
    });
});

describe("ObjectSerdeable", () => {
    const serdeable = new ObjectSerdeable({
        a: new Uint8Serdeable(),
        b: new BooleanSerdeable(),
    });

    it("serializes object", () => {
        const writer = new TestWriter(2);
        const serializer = serdeable.serializer({ a: 2, b: true });
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([2, 1]));
    });

    it("deserializes object", () => {
        const reader = new TestReader([2, 1]);
        const deserializer = serdeable.deserializer();
        expect(deserializer.deserialize(reader).unwrap()).toEqual({ a: 2, b: true });
    });
});
