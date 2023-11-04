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
            DEBUG_ASSERT(is_unicast());
            return etl::get<LinkUnicastAddress>(address_);
        }

        inline const LinkBroadcastAddress &unwrap_broadcast() const {
            DEBUG_ASSERT(is_broadcast());
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

    struct [[deprecated("use LinkFrame")]] Frame {
        frame::ProtocolNumber protocol_number;
        Address peer;
        uint8_t length;
        frame::FrameBufferReader reader;
    };

    struct [[deprecated("use LinkFrame")]] SendingFrame {
        frame::ProtocolNumber protocol_number;
        Address peer;
        frame::FrameBufferReader &&reader_ref;
    };

    struct [[deprecated("use LinkFrame")]] ReceivedFrame {
        Address peer;
        frame::FrameBufferReader reader;
    };

    template <typename Media, typename FrameService>
    concept IMedia = frame::IFrameService<FrameService> &&
        requires(Media &media, FrameService &frame_service, Frame &frame) {
            { media.wait_for_media_detection() } -> util::same_as<nb::Poll<void>>;
            { media.get_address() } -> util::same_as<etl::optional<typename FrameService::Address>>;
            { media.send_frame(etl::move(frame)) } -> util::same_as<nb::Poll<void>>;
            { media.receive_frame() } -> util::same_as<nb::Poll<Frame>>;
            { media.execute(frame_service) } -> util::same_as<void>;
        };
} // namespace net::link
