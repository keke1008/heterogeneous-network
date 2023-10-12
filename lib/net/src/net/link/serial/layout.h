#pragma once

#include "../address.h"
#include "./address.h"
#include <nb/buf.h>
#include <net/frame/service.h>
#include <stdint.h>

namespace net::link::serial {
    // # シリアル通信のフレームレイアウト
    //
    // 1. プリアンブル(8byte)
    // 2. 上位プロトコル(1byte)
    // 3. 送信元アドレス(1byte)
    // 4. 宛先アドレス(1byte)
    // 5. データ長(1byte)
    // 6. データ

    constexpr uint8_t HEADER_LENGTH =
        frame::PROTOCOL_SIZE + SerialAddress::SIZE * 2 + frame::BODY_LENGTH_SIZE;

    constexpr uint8_t PREAMBLE = 0b10101010;
    constexpr uint8_t PREAMBLE_LENGTH = 8;

    struct FrameHeader {
        frame::ProtocolNumber protocol_number;
        SerialAddress source;
        SerialAddress destination;
        uint8_t length;

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(static_cast<uint8_t>(protocol_number));
            builder.append(source);
            builder.append(destination);
            builder.append(length);
        }
    };

    struct FrameHeaderParser {
        FrameHeader parse(nb::buf::BufferSplitter &splitter) {
            return FrameHeader{
                .protocol_number = static_cast<frame::ProtocolNumber>(splitter.split_1byte()),
                .source = splitter.parse<SerialAddressParser>(),
                .destination = splitter.parse<SerialAddressParser>(),
                .length = splitter.split_1byte(),
            };
        }
    };
} // namespace net::link::serial
