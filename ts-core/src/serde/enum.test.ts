import { EnumSerdeable } from "./enum";
import { TestReader, TestWriter } from "./helper.test";

enum TestEnum {
    A = 1,
    B = 2,
    C = 3,
}

describe("EnumSerializer", () => {
    it("serializes enum", () => {
        const writer = new TestWriter(1);
        const serializer = new EnumSerdeable<TestEnum>(TestEnum).serializer(TestEnum.A);
        expect(serializer.serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([TestEnum.A]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(0);
        const serializer = new EnumSerdeable<TestEnum>(TestEnum).serializer(TestEnum.A);
        expect(serializer.serialize(writer).isOk()).toBe(false);
    });
});

describe("EnumDeserializer", () => {
    it("deserializes enum", () => {
        const reader = new TestReader([TestEnum.A]);
        const deserializer = new EnumSerdeable<TestEnum>(TestEnum).deserializer();
        expect(deserializer.deserialize(reader).unwrap()).toEqual(TestEnum.A);
    });

    it("fails to deserialize if there is not enough data", () => {
        const reader = new TestReader([]);
        const deserializer = new EnumSerdeable<TestEnum>(TestEnum).deserializer();
        expect(deserializer.deserialize(reader).isErr()).toBe(true);
    });
});
