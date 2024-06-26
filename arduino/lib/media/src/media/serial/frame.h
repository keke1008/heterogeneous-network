#pragma once

#include "../address/serial.h"
#include <net/frame.h>
#include <net/link.h>
#include <stdint.h>

namespace media::serial {
    class AsyncSerialAddressDeserializer {
        nb::de::Bin<uint8_t> address_;

      public:
        inline SerialAddress result() const {
            return SerialAddress{address_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return address_.deserialize(reader);
        }
    };

    class AsyncSerialAddressSerializer {
        nb::ser::Bin<uint8_t> address_;

      public:
        inline AsyncSerialAddressSerializer(const SerialAddress &address)
            : address_{address.get()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writer) {
            return address_.serialize(writer);
        }

        inline constexpr uint8_t serialized_length() const {
            return address_.serialized_length();
        }
    };

    // # シリアル通信のフレームレイアウト
    //
    // 1. プリアンブル(8byte)
    // 2. 上位プロトコル(1byte)
    // 3. 送信元アドレス(1byte)
    // 4. 宛先アドレス(1byte)
    // 5. データ長(1byte)
    // 6. データ

    constexpr uint8_t HEADER_LENGTH =
        net::frame::PROTOCOL_SIZE + SerialAddress::SIZE * 2 + net::frame::BODY_LENGTH_SIZE;

    constexpr uint8_t PREAMBLE = 0b10101010;
    constexpr uint8_t PREAMBLE_LENGTH = 7;
    constexpr uint8_t LAST_PREAMBLE = 0b10101011;
    constexpr uint8_t LAST_PREAMBLE_LENGTH = 1;

    struct SerialFrameHeader {
        net::frame::ProtocolNumber protocol_number;
        SerialAddress source;
        SerialAddress destination;
        uint8_t length;
    };

    class AsyncSerialFrameHeaderSerializer {
        nb::ser::Bin<uint8_t> protocol_number_;
        AsyncSerialAddressSerializer source_;
        AsyncSerialAddressSerializer destination_;
        nb::ser::Bin<uint8_t> length_;

      public:
        inline AsyncSerialFrameHeaderSerializer(const SerialFrameHeader &header)
            : protocol_number_(static_cast<uint8_t>(header.protocol_number)),
              source_(header.source),
              destination_(header.destination),
              length_(header.length) {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(protocol_number_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(destination_.serialize(writer));
            return length_.serialize(writer);
        }

        inline constexpr uint8_t serialized_length() const {
            return protocol_number_.serialized_length() + source_.serialized_length() +
                destination_.serialized_length() + length_.serialized_length();
        }
    };

    class AsyncSerialFrameHeaderDeserializer {
        net::frame::AsyncProtocolNumberDeserializer protocol_number_;
        AsyncSerialAddressDeserializer source_;
        AsyncSerialAddressDeserializer destination_;
        nb::de::Bin<uint8_t> length_;

      public:
        inline SerialFrameHeader result() {
            return SerialFrameHeader{
                .protocol_number = protocol_number_.result(),
                .source = source_.result(),
                .destination = destination_.result(),
                .length = length_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(protocol_number_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(destination_.deserialize(reader));
            return length_.deserialize(reader);
        }
    };
} // namespace media::serial
