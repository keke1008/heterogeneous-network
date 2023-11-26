#pragma once

#include "../address.h"
#include "../media.h"
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/span.h>
#include <nb/buf.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <serde/dec.h>
#include <util/span.h>

namespace net::link::wifi {
    class WifiIpV4Address {
        etl::array<uint8_t, 4> address_;

      public:
        explicit WifiIpV4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            : address_{a, b, c, d} {}

        explicit WifiIpV4Address(etl::span<const uint8_t> address) {
            ASSERT(address.size() == 4);
            etl::copy(address.begin(), address.end(), address_.data());
        }

        explicit WifiIpV4Address(const Address &address) {
            ASSERT(address.type() == AddressType::IPv4);
            ASSERT(address.address().size() == 6);
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
    };

    class AsyncWifiIpV4AddressSerializer {
        nb::ser::Array<nb::ser::Dec<uint8_t>, 4> address_;

      public:
        explicit AsyncWifiIpV4AddressSerializer(const WifiIpV4Address &address)
            : address_{address.address()} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return address_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return address_.serialized_length();
        }
    };

    class AsyncWifiIpV4AddressDeserializer {
        nb::de::Array<nb::de::Dec<uint8_t>, 4> address_{4};

      public:
        inline WifiIpV4Address result() const {
            return WifiIpV4Address{address_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return address_.deserialize(reader);
        }
    };

    class WifiPort {
        uint16_t port_;

      public:
        explicit WifiPort(uint16_t port) : port_{port} {}

        explicit WifiPort(const Address &address) {
            ASSERT(address.type() == AddressType::IPv4);
            ASSERT(address.address().size() == 6);
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
    };

    class AsyncWifiPortSerializer {
        nb::ser::Dec<uint16_t> port_;

      public:
        explicit AsyncWifiPortSerializer(WifiPort port) : port_{port.port()} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return port_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return port_.serialized_length();
        }
    };

    class AsyncWifiPortDeserializer {
        nb::de::Dec<uint16_t> port_;

      public:
        inline WifiPort result() const {
            return WifiPort{port_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return port_.deserialize(reader);
        }
    };

    class WifiAddress {
        WifiIpV4Address ip_;
        WifiPort port_;

        etl::array<uint8_t, 6> serialize() const {
            etl::array<uint8_t, 6> addr{};
            auto ipaddr = ip_.address();
            addr.assign(ipaddr.begin(), ipaddr.end());

            auto result = nb::serialize_span(
                etl::span{addr}.subspan(4, 2), nb::ser::Bin<uint16_t>{port_.port()}
            );
            ASSERT(result.is_ready() && result.unwrap() == nb::SerializeResult::Ok);

            return addr;
        }

      public:
        explicit WifiAddress(WifiIpV4Address ip, WifiPort port) : ip_{ip}, port_{port} {}

        explicit WifiAddress(const Address &address)
            : ip_{([&]() {
                  ASSERT(address.type() == AddressType::IPv4);
                  ASSERT(address.address().size() == 6);
                  return address.address().subspan(0, 4);
              })()},
              port_{serde::bin::deserialize<uint16_t>(address.address().subspan(4, 2))} {}

        explicit WifiAddress(const LinkAddress &address)
            : WifiAddress{([&]() {
                  ASSERT(address.is_unicast());
                  return address.unwrap_unicast().address;
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

    class AsyncWifiAddressDeserializer {
        AsyncWifiIpV4AddressDeserializer ip_;
        AsyncWifiPortDeserializer port_;

      public:
        inline WifiAddress result() const {
            return WifiAddress{ip_.result(), port_.result()};
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(ip_.deserialize(reader));
            return port_.deserialize(reader);
        }
    };

    struct WifiFrame {
        frame::ProtocolNumber protocol_number;
        WifiAddress remote;
        frame::FrameBufferReader reader;

        static WifiFrame from_link_frame(LinkFrame &&frame) {
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
