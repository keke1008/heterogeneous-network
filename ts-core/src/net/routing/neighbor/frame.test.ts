import { Address, SerialAddress, Protocol } from "@core/net/link";
import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Cost, NodeId } from "../node";
import { FrameType, GoodbyeFrame, HelloFrame, deserializeFrame, serializeFrame } from "./frame";

describe("HelloFrame", () => {
    const senderId = NodeId.fromAddress(new SerialAddress(0x01));
    const edgeCost = new Cost(8);

    it("deserialize", () => {
        const buffer = new Uint8Array(255);
        const writer = new BufferWriter(buffer);
        senderId.serialize(writer);
        edgeCost.serialize(writer);

        const type = FrameType.Hello;
        const frame = HelloFrame.deserialize(new BufferReader(writer.shrinked()), type);
        expect(frame.type).toBe(type);
        expect(frame.senderId.equals(senderId)).toBe(true);
        expect(frame.edgeCost.equals(edgeCost)).toBe(true);
    });

    it("serialize", () => {
        const frame = new HelloFrame({ type: FrameType.Hello, senderId, edgeCost });
        const buffer = new Uint8Array(255);
        const writer = new BufferWriter(buffer);
        frame.serialize(writer);

        const reader = new BufferReader(writer.shrinked());

        const type = reader.readByte();
        expect(type).toBe(FrameType.Hello);

        const deserialized = HelloFrame.deserialize(reader, type);
        expect(deserialized.type).toBe(type);
        expect(deserialized.senderId.equals(senderId)).toBe(true);
        expect(deserialized.edgeCost.equals(edgeCost)).toBe(true);
    });
});

describe("GoodbyeFrame", () => {
    const senderId = NodeId.fromAddress(new SerialAddress(0x01));

    it("deserialize", () => {
        const buffer = new Uint8Array(255);
        const writer = new BufferWriter(buffer);
        senderId.serialize(writer);

        const type = FrameType.Goodbye;
        const frame = GoodbyeFrame.deserialize(new BufferReader(writer.shrinked()));
        expect(frame.type).toBe(type);
        expect(frame.senderId.equals(senderId)).toBe(true);
    });

    it("serialize", () => {
        const frame = new GoodbyeFrame({ senderId });
        const buffer = new Uint8Array(255);
        const writer = new BufferWriter(buffer);
        frame.serialize(writer);

        const reader = new BufferReader(writer.shrinked());

        const type = reader.readByte();
        expect(type).toBe(FrameType.Goodbye);

        const deserialized = GoodbyeFrame.deserialize(reader);
        expect(deserialized.type).toBe(type);
        expect(deserialized.senderId.equals(senderId)).toBe(true);
    });
});

describe("Frame", () => {
    const senderId = NodeId.fromAddress(new SerialAddress(0x01));
    it("deserialize", () => {
        const buffer = new Uint8Array(255);
        const writer = new BufferWriter(buffer);
        new GoodbyeFrame({ senderId }).serialize(writer);

        const result = deserializeFrame(new BufferReader(writer.shrinked()));
        expect(result.type).toBe(FrameType.Goodbye);
    });

    it("serialize", () => {
        const frame = new GoodbyeFrame({ senderId });
        const result = serializeFrame(frame, new Address(new SerialAddress(0x01)));
        expect(result.protocol).toBe(Protocol.RoutingNeighbor);
    });
});
