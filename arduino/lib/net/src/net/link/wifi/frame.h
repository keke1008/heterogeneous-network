#pragma once

#include "../address.h"
#include "../media.h"
#include <debug_assert.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/span.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame.h>
#include <serde/dec.h>
#include <util/span.h>

namespace net::link::wifi {
    class WifiIpV4Address {
        etl::array<uint8_t, 4> address_;

      public:
        explicit WifiIpV4Address(etl::span<const uint8_t> address) {
            DEBUG_ASSERT(address.size() == 4);
            etl::copy(address.begin(), address.end(), address_.data());
        }

        explicit WifiIpV4Address(const Address &address) {
            DEBUG_ASSERT(address.type() == AddressType::IPv4);
            DEBUG_ASSERT(address.address().size() == 6);
            address_.assign(address.address().begin(), address.address().begin() + 4);
        }

        inline bool operator==(const WifiIpV4Address &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const WifiIpV4Address &other) const {
            return address_ != other.address_;
        }

        etl::span<const uint8_t, 4> address() const {
            return address_;
        }

        void write_to_builder(nb::buf::BufferBuilder &builder) {
            DEBUG_ASSERT(builder.writable_count() >= 15);

            auto write_byte = [](uint8_t byte) {
                return [byte](auto span) { return serde::dec::serialize(span, byte); };
            };

            for (uint8_t i = 0; i < 3; ++i) {
                builder.append(write_byte(address_[i]));
                builder.append(static_cast<uint8_t>('.'));
            }
            builder.append(write_byte(address_[3]));
        }
    };

    struct WifiIpV4AddressDeserializer {
        inline WifiIpV4Address parse(nb::buf::BufferSplitter &splitter) {
            etl::array<uint8_t, 4> address{0, 0, 0, 0};
            for (uint8_t i = 0; i < 3; i++) {
                auto part = splitter.split_sentinel('.');
                address[i] = serde::dec::deserialize<uint8_t>(part);
            }

            auto last_part = splitter.split_remaining();
            address[3] = serde::dec::deserialize<uint8_t>(last_part);
            return WifiIpV4Address{address};
        }
    };

    class WifiPort {
        uint16_t port_;

      public:
        explicit WifiPort(uint16_t port) : port_{port} {}

        explicit WifiPort(const Address &address) {
            DEBUG_ASSERT(address.type() == AddressType::IPv4);
            DEBUG_ASSERT(address.address().size() == 6);
            port_ = serde::bin::deserialize<uint16_t>(address.address().subspan(4, 2));
        }

        inline bool operator==(const WifiPort &other) const {
            return port_ == other.port_;
        }

        inline bool operator!=(const WifiPort &other) const {
            return port_ != other.port_;
        }

        uint16_t port() const {
            return port_;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) {
            builder.append(nb::buf::FormatDecimal<uint16_t>{port_});
        }
    };

    struct WifiPortDeserializer {
        inline WifiPort parse(nb::buf::BufferSplitter &splitter) {
            return WifiPort{serde::dec::deserialize<uint16_t>(splitter.split_nbytes(2))};
        }
    };

    class WifiAddress {
        WifiIpV4Address ip_;
        WifiPort port_;

        etl::array<uint8_t, 6> serialize() const {
            etl::array<uint8_t, 6> result{};
            nb::buf::BufferBuilder builder{result};
            builder.append(ip_.address());
            builder.append(nb::buf::FormatBinary<uint16_t>{port_.port()});
            return result;
        }

      public:
        explicit WifiAddress(WifiIpV4Address ip, WifiPort port) : ip_{ip}, port_{port} {}

        explicit WifiAddress(const Address &address)
            : ip_{([&]() {
                  DEBUG_ASSERT(address.type() == AddressType::IPv4);
                  DEBUG_ASSERT(address.address().size() == 6);
                  return address.address().subspan(0, 4);
              })()},
              port_{serde::bin::deserialize<uint16_t>(address.address().subspan(4, 2))} {}

        explicit WifiAddress(const LinkAddress &address)
            : WifiAddress{([&]() {
                  DEBUG_ASSERT(address.is_unicast());
                  return address.get_unicast().remote;
              })()} {}

        explicit operator Address() const {
            return Address{AddressType::IPv4, serialize()};
        }

        explicit operator LinkAddress() const {
            return LinkAddress{Address{*this}};
        }

        inline bool operator==(const WifiAddress &other) const {
            return ip_ == other.ip_ && port_ == other.port_;
        }

        inline bool operator!=(const WifiAddress &other) const {
            return ip_ != other.ip_ || port_ != other.port_;
        }

        inline etl::span<const uint8_t, 4> address() const {
            return ip_.address();
        }

        inline const WifiIpV4Address &address_part() const {
            return ip_;
        }

        inline uint16_t port() const {
            return port_.port();
        }

        inline const WifiPort &port_part() const {
            return port_;
        }
    };

    struct WifiAddressDeserializer {
        uint8_t delimiter;

        WifiAddressDeserializer() = delete;

        explicit WifiAddressDeserializer(uint8_t delimiter) : delimiter{delimiter} {}

        inline WifiAddress parse(nb::buf::BufferSplitter &splitter) {
            auto ip_part = splitter.split_sentinel(delimiter);
            auto ip = nb::buf::BufferSplitter{ip_part}.parse<WifiIpV4AddressDeserializer>();
            auto port = splitter.parse<WifiPortDeserializer>();
            return WifiAddress{ip, port};
        }
    };

    struct WifiFrame {
        frame::ProtocolNumber protocol_number;
        WifiAddress remote;
        frame::FrameBufferReader reader;

        static WifiFrame from_link_frame(LinkFrame &frame) {
            return WifiFrame{
                .protocol_number = frame.protocol_number,
                .remote = WifiAddress{frame.remote},
                .reader = etl::move(frame.reader),
            };
        }

        explicit operator LinkFrame() && {
            return LinkFrame{
                .protocol_number = protocol_number,
                .remote = LinkAddress{remote},
                .reader = etl::move(reader),
            };
        }
    };
} // namespace net::link::wifi
