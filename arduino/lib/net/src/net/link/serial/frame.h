#pragma once

#include "../address.h"
#include "../media.h"
#include <nb/buf.h>
#include <net/frame/service.h>
#include <stdint.h>

namespace net::link::serial {
    class SerialAddress {
        uint8_t address_;

      public:
        static constexpr uint8_t SIZE = 1;

        explicit SerialAddress(uint8_t address) : address_{address} {}

        explicit SerialAddress(const Address &address) {
            ASSERT(address.type() == AddressType::Serial);
            ASSERT(address.address().size() == 1);
            address_ = address.address()[0];
        }

        explicit SerialAddress(const LinkAddress &address)
            : SerialAddress(address.unwrap_unicast().address) {}

        explicit operator Address() const {
            return Address{AddressType::Serial, {address_}};
        }

        explicit operator LinkAddress() const {
            return LinkAddress{Address{*this}};
        }

        inline bool operator==(const SerialAddress &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const SerialAddress &other) const {
            return !(*this == other);
        }

        uint8_t get() const {
            return address_;
        }
    };

    class AsyncSerialAddressDeserializer {
        nb::de::Bin<uint8_t> address_;

      public:
        inline SerialAddress result() {
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
        frame::PROTOCOL_SIZE + SerialAddress::SIZE * 2 + frame::BODY_LENGTH_SIZE;

    constexpr uint8_t PREAMBLE = 0b10101010;
    constexpr uint8_t PREAMBLE_LENGTH = 8;

    struct SerialFrameHeader {
        frame::ProtocolNumber protocol_number;
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
        nb::de::Bin<uint8_t> protocol_number_;
        AsyncSerialAddressDeserializer source_;
        AsyncSerialAddressDeserializer destination_;
        nb::de::Bin<uint8_t> length_;

      public:
        inline SerialFrameHeader result() {
            return SerialFrameHeader{
                .protocol_number = static_cast<frame::ProtocolNumber>(protocol_number_.result()),
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
} // namespace net::link::serial
