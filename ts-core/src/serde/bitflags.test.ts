import { BitflagsDeserailizer } from "./bitflags";
import { TestReader } from "./helper.test";
import { Uint8Deserializer } from "./primitives";

enum TestFlags {
    A = 1 << 0,
    B = 1 << 1,
    C = 1 << 2,
    D = 1 << 3,
}

describe("BitflagsDeserializer", () => {
    it.each([
        ["a single flag", TestFlags.A],
        ["multiple flags", TestFlags.A | TestFlags.B],
        ["all flags", TestFlags.A | TestFlags.B | TestFlags.C | TestFlags.D],
    ] as const)("should deserialize %s", (_, value) => {
        const reader = new TestReader([value]);
        const deserializer = new BitflagsDeserailizer<TestFlags>(TestFlags, new Uint8Deserializer());
        expect(deserializer.deserialize(reader).unwrap()).toEqual(value);
    });

    it("should fail if the value is not a valid flag", () => {
        const reader = new TestReader([0xff]);
        const deserializer = new BitflagsDeserailizer<TestFlags>(TestFlags, new Uint8Deserializer());
        expect(deserializer.deserialize(reader).isErr()).toBe(true);
    });
});
