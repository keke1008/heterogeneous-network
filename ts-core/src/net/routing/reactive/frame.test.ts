import { SerialAddress } from "@core/net/link";
import { BufferReader, BufferWriter } from "../../buffer";
import { Cost, NodeId } from "../node";
import { RouteDiscoveryFrame, RouteDiscoveryFrameType, deserializeFrame, serializeFrame } from "./frame";
import { FrameId } from "./frameId";

describe("RouteDiscoveryFrame", () => {
    const frameId = new FrameId(0x01);
    const totalCost = new Cost(0x02);
    const sourceId = NodeId.fromAddress(new SerialAddress(0x03));
    const targetId = NodeId.fromAddress(new SerialAddress(0x04));
    const senderId = NodeId.fromAddress(new SerialAddress(0x05));

    it("deserialize", () => {
        const writer = new BufferWriter(new Uint8Array(255));

        const type = RouteDiscoveryFrameType.Request;
        writer.writeByte(type);
        frameId.serialize(writer);
        totalCost.serialize(writer);
        sourceId.serialize(writer);
        targetId.serialize(writer);
        senderId.serialize(writer);

        const frame = deserializeFrame(new BufferReader(writer.shrinked()));
        expect(frame.type).toBe(type);
        expect(frame.frameId.equals(frameId)).toBe(true);
        expect(frame.totalCost.equals(totalCost)).toBe(true);
        expect(frame.sourceId.equals(sourceId)).toBe(true);
        expect(frame.targetId.equals(targetId)).toBe(true);
        expect(frame.senderId.equals(senderId)).toBe(true);
    });

    it("serialize", () => {
        const type = RouteDiscoveryFrameType.Reply;
        const frame = new RouteDiscoveryFrame(type, frameId, totalCost, sourceId, targetId, senderId);
        const reader = serializeFrame(frame);
        const actual = deserializeFrame(reader);

        expect(actual.type).toBe(type);
        expect(actual.frameId.equals(frameId)).toBe(true);
        expect(actual.totalCost.equals(totalCost)).toBe(true);
        expect(actual.sourceId.equals(sourceId)).toBe(true);
        expect(actual.targetId.equals(targetId)).toBe(true);
        expect(actual.senderId.equals(senderId)).toBe(true);
    });
});
