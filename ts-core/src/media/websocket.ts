import { WebSocketAddress, Address, Frame, numberToProtocol, protocolToNumber } from "../net/link";
import { BufferReader, BufferWriter } from "../net/buffer";

export const deserializeWebSocketFrame = (data: BufferReader): Frame => {
    return {
        protocol: numberToProtocol(data.readByte()),
        sender: new Address(WebSocketAddress.deserialize(data)),
        reader: data,
    };
};

export const serializeWebSocketFrame = (frame: Frame): Uint8Array => {
    if (!(frame.sender.address instanceof WebSocketAddress)) {
        throw new Error(`Expected WebsocketAddress, got ${frame.sender}`);
    }

    const length = 1 + frame.sender.address.serializedLength() + frame.reader.readableLength();
    const writer = new BufferWriter(new Uint8Array(length));

    writer.writeByte(protocolToNumber(frame.protocol));
    frame.sender.address.serialize(writer);
    writer.writeBytes(frame.reader.readRemaining());
    return writer.unwrapBuffer();
};
