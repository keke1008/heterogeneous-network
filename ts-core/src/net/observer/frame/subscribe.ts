import { BufferReader, BufferWriter } from "@core/net/buffer";
import { FRAME_TYPE_SERIALIXED_LENGTH, FrameType } from "./common";
import { FrameId } from "@core/net/link";
import { DeserializeResult } from "@core/serde";

export class SubscribeFrame {
    readonly type = FrameType.Subscribe;
    readonly frameId: FrameId;

    constructor(args: { frameId: FrameId }) {
        this.frameId = args.frameId;
    }

    static deserialize(reader: BufferReader): DeserializeResult<SubscribeFrame> {
        return FrameId.deserialize(reader).map((frameId) => new SubscribeFrame({ frameId }));
    }

    serialize(writer: BufferWriter): void {
        writer.writeByte(this.type);
        this.frameId.serialize(writer);
    }

    serializedLength(): number {
        return FRAME_TYPE_SERIALIXED_LENGTH + this.frameId.serializedLength();
    }
}
