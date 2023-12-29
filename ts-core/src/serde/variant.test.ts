import { TestReader, TestWriter } from "./helper.test";
import { BooleanSerdeable, Uint8Serdeable } from "./primitives";
import { ManualVariantSerdeable, VariantSerdeable } from "./variant";

const toIndex = (v: number | boolean) => (typeof v === "number" ? 1 : 2);
const normalSerdeable = new VariantSerdeable([new Uint8Serdeable(), new BooleanSerdeable()] as const, toIndex);

const manualSerdeable = new ManualVariantSerdeable(
    (v) => (typeof v === "number" ? 1 : 2),
    (id) => (id === 1 ? new Uint8Serdeable() : new BooleanSerdeable()),
);

describe.each([
    ["VariantSerdeable", normalSerdeable],
    ["ManualVariantSerdeabl", manualSerdeable],
])("%p", (_, serdeable) => {
    it("serializes variant 0", () => {
        const writer = new TestWriter(2);
        expect(serdeable.serializer(2).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([1, 2]));
    });

    it("serializes variant 1", () => {
        const writer = new TestWriter(2);
        expect(serdeable.serializer(true).serialize(writer).isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([2, 1]));
    });

    it("fails to serialize if there is not enough space", () => {
        const writer = new TestWriter(1);
        expect(serdeable.serializer(true).serialize(writer).isOk()).toBe(false);
    });

    it("deserializes variant 0", () => {
        const reader = new TestReader([1, 2]);
        expect(serdeable.deserializer().deserialize(reader).unwrap()).toEqual(2);
    });

    it("deserializes variant 1", () => {
        const reader = new TestReader([2, 1]);
        expect(serdeable.deserializer().deserialize(reader).unwrap()).toEqual(true);
    });

    it("fails to deserialize if there is not enough data", () => {
        const reader = new TestReader([1]);
        expect(serdeable.deserializer().deserialize(reader).isErr()).toBe(true);
    });
});
