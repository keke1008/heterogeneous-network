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

    class MediaPortNumber {
        uint8_t value_;

      public:
        explicit constexpr MediaPortNumber(uint8_t value) : value_{value} {}

        inline constexpr bool operator==(const MediaPortNumber &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const MediaPortNumber &other) const {
            return value_ != other.value_;
        }

        inline constexpr uint8_t value() const {
            return value_;
        }
    };

    class AsyncMediaPortNumberDeserializer {
        nb::de::Bin<uint8_t> value_;

      public:
        inline MediaPortNumber result() const {
            return MediaPortNumber{value_.result()};
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            return value_.deserialize(readable);
        }
    };

    class AsyncMediaPortNumberSerializer {
        nb::ser::Bin<uint8_t> value_;

      public:
        inline explicit AsyncMediaPortNumberSerializer(MediaPortNumber port)
            : value_{port.value()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            return value_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return value_.serialized_length();
        }
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

    struct LinkFrame {
        frame::ProtocolNumber protocol_number;
        Address remote;
        frame::FrameBufferReader reader;
    };
} // namespace net::link
