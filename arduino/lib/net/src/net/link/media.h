#pragma once

#include "./address.h"
#include <net/frame/service.h>
#include <stdint.h>
#include <util/time.h>

namespace net::link {
    enum class MediaType : uint8_t {
        UHF,
        Wifi,
        Serial,
    };

    struct LinkUnicastAddress {
        Address address;
    };

    struct LinkBroadcastAddress {
        AddressType type;
    };

    struct MediaInfo {
        etl::optional<AddressType> address_type;
        etl::optional<Address> address;
    };

    class LinkAddress {
        etl::variant<LinkUnicastAddress, LinkBroadcastAddress> address_;

      public:
        explicit LinkAddress(const LinkUnicastAddress &address) : address_{etl::move(address)} {}

        explicit LinkAddress(const Address &address)
            : address_{LinkUnicastAddress{.address = address}} {}

        explicit LinkAddress(LinkBroadcastAddress address) : address_{etl::move(address)} {}

        explicit LinkAddress(AddressType type) : address_{LinkBroadcastAddress{.type = type}} {}

        inline bool is_unicast() const {
            return etl::holds_alternative<LinkUnicastAddress>(address_);
        }

        inline bool is_broadcast() const {
            return etl::holds_alternative<LinkBroadcastAddress>(address_);
        }

        inline const LinkUnicastAddress &unwrap_unicast() const {
            ASSERT(is_unicast());
            return etl::get<LinkUnicastAddress>(address_);
        }

        inline const LinkBroadcastAddress &unwrap_broadcast() const {
            ASSERT(is_broadcast());
            return etl::get<LinkBroadcastAddress>(address_);
        }

        inline AddressType address_type() const {
            return etl::visit(
                util::Visitor{
                    [](const LinkUnicastAddress &address) { return address.address.type(); },
                    [](const LinkBroadcastAddress &address) { return address.type; },
                },
                address_
            );
        }

        inline etl::variant<LinkUnicastAddress, LinkBroadcastAddress> &variant() {
            return address_;
        }

        inline const etl::variant<LinkUnicastAddress, LinkBroadcastAddress> &variant() const {
            return address_;
        }
    };

    struct LinkFrame {
        frame::ProtocolNumber protocol_number;
        LinkAddress remote;
        frame::FrameBufferReader reader;
    };
} // namespace net::link
