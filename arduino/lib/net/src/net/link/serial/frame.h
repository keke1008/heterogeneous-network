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

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(address_);
        }
    };

    struct SerialAddressParser {
        inline SerialAddress parse(nb::buf::BufferSplitter &splitter) {
            return SerialAddress{splitter.split_1byte()};
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

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(static_cast<uint8_t>(protocol_number));
            builder.append(source);
            builder.append(destination);
            builder.append(length);
        }
    };

    struct SerialFrameHeaderParser {
        SerialFrameHeader parse(nb::buf::BufferSplitter &splitter) {
            return SerialFrameHeader{
                .protocol_number = static_cast<frame::ProtocolNumber>(splitter.split_1byte()),
                .source = splitter.parse<SerialAddressParser>(),
                .destination = splitter.parse<SerialAddressParser>(),
                .length = splitter.split_1byte(),
            };
        }
    };
} // namespace net::link::serial
