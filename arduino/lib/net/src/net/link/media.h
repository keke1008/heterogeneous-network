#pragma once

#include "./address.h"
#include <nb/serde.h>
#include <net/frame.h>
#include <stdint.h>
#include <util/time.h>
#include <util/visitor.h>

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

    class AsyncMediaInfoSerializer {
        nb::ser::Optional<link::AsyncAddressTypeSerializer> address_type;
        nb::ser::Optional<link::AsyncAddressSerializer> address;

      public:
        inline explicit AsyncMediaInfoSerializer(const MediaInfo &info)
            : address_type{info.address_type},
              address{info.address} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            SERDE_SERIALIZE_OR_RETURN(address_type.serialize(writable));
            return address.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return address_type.serialized_length() + address.serialized_length();
        }
    };

    class AsyncMediaInfoDeserializer {
        nb::de::Optional<link::AsyncAddressTypeDeserializer> address_type;
        nb::de::Optional<link::AsyncAddressDeserializer> address;

      public:
        inline MediaInfo result() const {
            return MediaInfo{
                .address_type = address_type.result(),
                .address = address.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserializer(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(address_type.deserialize(readable));
            return address.deserialize(readable);
        }
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
