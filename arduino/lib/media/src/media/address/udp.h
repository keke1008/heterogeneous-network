#pragma once

#include <etl/array.h>
#include <etl/span.h>
#include <logger.h>
#include <nb/serde.h>
#include <net/link.h>

namespace media {
    class UdpIpAddress {
        etl::array<uint8_t, 4> address_;

      public:
        explicit UdpIpAddress(etl::span<const uint8_t> address) {
            FASSERT(address.size() == 4);
            etl::copy_n(address.begin(), 4, address_.begin());
        }

        inline bool operator==(const UdpIpAddress &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const UdpIpAddress &other) const {
            return address_ != other.address_;
        }

        inline etl::span<const uint8_t, 4> body() const {
            return address_;
        }
    };

    class UdpPort {
        uint16_t value_;

      public:
        explicit constexpr UdpPort(uint16_t value) : value_{value} {}

        explicit UdpPort(etl::span<const uint8_t> span)
            : value_{nb::deserialize_span_at_once(span, nb::de::Bin<uint16_t>{})} {
            FASSERT(span.size() == 2);
        }

        inline constexpr bool operator==(const UdpPort &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const UdpPort &other) const {
            return value_ != other.value_;
        }

        inline constexpr uint16_t value() const {
            return value_;
        }

        inline static UdpPort from_span(etl::span<const uint8_t> span) {
            FASSERT(span.size() == 2);
            return UdpPort{nb::deserialize_span_at_once(span, nb::de::Bin<uint16_t>{})};
        }

        inline void to_span(etl::span<uint8_t> dest) const {
            nb::serialize_span_at_once(dest, nb::ser::Bin<uint16_t>{value_});
        }
    };

    class UdpAddress {
        UdpIpAddress address_;
        UdpPort port_;

      public:
        explicit UdpAddress(const UdpIpAddress &address, UdpPort port)
            : address_{address},
              port_{port} {}

        explicit UdpAddress(etl::span<const uint8_t> spane)
            : address_{spane.subspan(0, 4)},
              port_{spane.subspan(4, 2)} {
            FASSERT(spane.size() == 6);
        }

        explicit UdpAddress(const net::link::Address &address) : UdpAddress{address.body()} {
            FASSERT(address.type() == net::link::AddressType::IPv4);
        }

        explicit operator net::link::Address() const {
            etl::array<uint8_t, 6> addr{};
            auto ipaddr = address_.body();
            addr.assign(ipaddr.begin(), ipaddr.end());

            auto port_span = etl::span{addr}.subspan(4, 2);
            auto result = nb::serialize_span(port_span, nb::ser::Bin<uint16_t>{port_.value()});
            FASSERT(result.is_ready() && result.unwrap() == nb::SerializeResult::Ok);

            return net::link::Address{net::link::AddressType::IPv4, addr};
        }

        inline bool operator==(const UdpAddress &other) const {
            return address_ == other.address_ && port_ == other.port_;
        }

        inline bool operator!=(const UdpAddress &other) const {
            return address_ != other.address_ || port_ != other.port_;
        }

        inline const UdpIpAddress &address() const {
            return address_;
        }

        inline UdpPort port() const {
            return port_;
        }
    };
} // namespace media
