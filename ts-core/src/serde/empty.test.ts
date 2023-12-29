import { EmptyDeserializer, EmptySerializer } from "./empty";
import { TestWriter } from "./helper.test";

describe("EmptySerializer", () => {
    it("serializes nothing", () => {
        const writer = new TestWriter(0);
        const serializer = new EmptySerializer();
        expect(serializer.serialize().isOk()).toBe(true);
        expect(writer.bytes).toEqual(new Uint8Array([]));
    });
});

describe("EmptyDeserializer", () => {
    it("deserializes nothing", () => {
        const deserializer = new EmptyDeserializer();
        expect(deserializer.deserialize().unwrap()).toBeUndefined();
    });
});
