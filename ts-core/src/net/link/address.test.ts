import { Address, SerialAddress, UdpAddress } from "./address";
import { BufferReader, BufferWriter } from "../buffer";

describe("SerialAddress", () => {
    it("deserialize", () => {
        const reader = new BufferReader(new Uint8Array([0x12]));
        const address = SerialAddress.deserialize(reader);
        expect(address.equals(new SerialAddress(0x12))).toBe(true);
    });

    it("serialize", () => {
        const writer = new BufferWriter(new Uint8Array(1));
        const address = new SerialAddress(0x12);
        address.serialize(writer);
        expect(writer.unwrapBuffer()).toStrictEqual(new Uint8Array([0x12]));
    });
});

describe("UhfAddress", () => {
    it("deserialize", () => {
        const reader = new BufferReader(new Uint8Array([0x12]));
        const address = SerialAddress.deserialize(reader);
        expect(address.equals(new SerialAddress(0x12))).toBe(true);
    });

    it("serialize", () => {
        const writer = new BufferWriter(new Uint8Array(1));
        const address = new SerialAddress(0x12);
        address.serialize(writer);
        expect(writer.unwrapBuffer()).toStrictEqual(new Uint8Array([0x12]));
    });
});

describe("UdpAddress", () => {
    const port = 0x1234;

    it("deserialize", () => {
        const reader = new BufferReader(new Uint8Array([192, 168, 0, 1, 0x34, 0x12]));
        const address = UdpAddress.deserialize(reader);
        expect(address.equals(new UdpAddress([192, 168, 0, 1], port))).toBe(true);
    });

    it("serialize", () => {
        const writer = new BufferWriter(new Uint8Array(6));
        const address = new UdpAddress([192, 168, 0, 1], port);
        address.serialize(writer);
        expect(writer.unwrapBuffer()).toStrictEqual(new Uint8Array([192, 168, 0, 1, 0x34, 0x12]));
    });
});

describe("Address", () => {
    it("deserialize", () => {
        const reader = new BufferReader(new Uint8Array([0x01, 0x12]));
        const address = Address.deserialize(reader);
        expect(address.equals(new Address(new SerialAddress(0x12)))).toBe(true);
    });

    it("serialize", () => {
        const writer = new BufferWriter(new Uint8Array(2));
        const address = new Address(new SerialAddress(0x12));
        address.serialize(writer);
        expect(writer.unwrapBuffer()).toStrictEqual(new Uint8Array([0x01, 0x12]));
    });
});
