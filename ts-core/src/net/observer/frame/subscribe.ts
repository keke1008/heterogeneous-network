import { BufferReader, BufferWriter } from "@core/net/buffer";
import { FrameType } from "./common";
import { FrameId } from "@core/net/link";
import { Ok, Result } from "oxide.ts";

export class SubscribeFrame {
    readonly type = FrameType.Subscribe;
    readonly frameId: FrameId;

    constructor(args: { frameId: FrameId }) {
        this.frameId = args.frameId;
    }

    static deserialize(reader: BufferReader): Result<SubscribeFrame, never> {
        const frameId = FrameId.deserialize(reader);
        return Ok(new SubscribeFrame({ frameId }));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.type);
        this.frameId.serialize(writer);
    }

    serializedLength(): number {
        return this.frameId.serializedLength();
    }
}
