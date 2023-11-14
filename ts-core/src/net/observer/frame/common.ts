import { BufferReader, BufferWriter } from "@core/net/buffer";
import { Err, Ok, Result } from "oxide.ts";

export enum FrameType {
    Subscribe = 0x01,
    Notify = 0x02,
}

export const FRAME_TYPE_SERIALIXED_LENGTH = 1;

export const serializeFrameType = (frameType: FrameType, writer: BufferWriter): void => {
    writer.writeByte(frameType);
};

export class InvalidNotifyFrameTypeError {
    constructor(public value: number) {}
}

export const deserializeFrameType = (reader: BufferReader): Result<FrameType, InvalidNotifyFrameTypeError> => {
    const frameType = reader.readByte();
    if (frameType in FrameType) {
        return Ok(frameType as FrameType);
    } else {
        return Err(new InvalidNotifyFrameTypeError(frameType));
    }
};
