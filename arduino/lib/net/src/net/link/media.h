#pragma once

#include "./address.h"
#include "./constants.h"
#include <etl/expected.h>
#include <nb/future.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <stdint.h>
#include <tl/concepts.h>
#include <util/time.h>
#include <util/visitor.h>

namespace net::link {
    enum class MediaType : uint8_t {
        UHF,
        Wifi,
        Serial,
    };

    class MediaPortMask;

    class MediaPortNumber {
        friend class MediaPortMask;
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

    class MediaPortMask {
        uint8_t mask_;

        explicit constexpr MediaPortMask(uint8_t mask) : mask_{mask} {}

        static inline constexpr uint8_t number_to_mask(MediaPortNumber number) {
            return 1 << number.value();
        }

        static constexpr uint8_t UNSPECIFIED = 0xff;

      public:
        MediaPortMask() = delete;

        static inline constexpr MediaPortMask zero() {
            return MediaPortMask{0};
        }

        static inline constexpr MediaPortMask unspecified() {
            return MediaPortMask{UNSPECIFIED};
        }

        inline bool is_unspecified() const {
            return mask_ == UNSPECIFIED;
        }

        static inline constexpr MediaPortMask from_port_number(MediaPortNumber number) {
            return MediaPortMask{number_to_mask(number)};
        }

        inline constexpr bool operator==(const MediaPortMask &other) const {
            return mask_ == other.mask_;
        }

        inline constexpr bool operator!=(const MediaPortMask &other) const {
            return mask_ != other.mask_;
        }

        inline constexpr MediaPortMask operator|=(const MediaPortMask &other) {
            mask_ |= other.mask_;
            return *this;
        }

        inline constexpr void set(MediaPortNumber number) {
            mask_ |= number_to_mask(number);
        }

        inline constexpr void reset(MediaPortNumber number) {
            mask_ &= ~(number_to_mask(number));
        }

        inline constexpr bool test(MediaPortNumber number) const {
            return (mask_ & (number_to_mask(number))) != 0;
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

    enum class MediaPortOperationResult {
        Success,
        Failure,
        UnsupportedOperation,
    };

    struct MediaPortUnsupportedOperation {};

    template <typename T>
    concept SerialMediaPort = requires(T t, const Address &address) {
        {
            t.serial_try_initialize_local_address(address)
        } -> tl::same_as<MediaPortOperationResult>;
    };

    template <typename T>
    concept WiFiSerialMediaPort = requires(
        T t,
        const Address &address,
        util::Time &time,
        etl::span<const uint8_t> ssid,
        etl::span<const uint8_t> password,
        uint16_t port
    ) {
        {
            t.wifi_join_ap(ssid, password, time)
        }
        -> tl::same_as<etl::expected<nb::Poll<nb::Future<bool>>, MediaPortUnsupportedOperation>>;
        {
            t.wifi_start_server(port, time)
        }
        -> tl::same_as<etl::expected<nb::Poll<nb::Future<bool>>, MediaPortUnsupportedOperation>>;
        {
            t.wifi_close_server(time)
        }
        -> tl::same_as<etl::expected<nb::Poll<nb::Future<bool>>, MediaPortUnsupportedOperation>>;
    };

    template <typename T>
    concept EthernetMediaPort = requires(T t, const etl::span<const uint8_t> &ip) {
        { t.ethernet_set_local_ip_address(ip) } -> tl::same_as<MediaPortOperationResult>;
        { t.ethernet_set_subnet_mask(ip) } -> tl::same_as<MediaPortOperationResult>;
    };

    template <typename T>
    concept MediaPort =
        SerialMediaPort<T> && WiFiSerialMediaPort<T> && EthernetMediaPort<T> && requires(T t) {
            { t.get_media_info() } -> tl::same_as<MediaInfo>;
        };

    template <typename T>
    concept MediaService = MediaPort<typename T::MediaPortType> &&
        requires(T t,
                 MediaPortNumber port,
                 AddressType type,
                 etl::vector<Address, MAX_MEDIA_PER_NODE> &addresses,
                 etl::vector<MediaInfo, MAX_MEDIA_PER_NODE> &media_info) {
            {
                t.get_media_port(port)
            } -> tl::same_as<etl::optional<etl::reference_wrapper<typename T::MediaPortType>>>;
            { t.get_media_address() } -> tl::same_as<etl::optional<Address>>;
            { t.get_media_addresses(addresses) } -> tl::same_as<void>;
            { t.get_media_info(media_info) } -> tl::same_as<void>;
            { t.get_broadcast_address(type) } -> tl::same_as<etl::optional<Address>>;
        };

    struct LinkFrame {
        MediaPortMask media_port_mask;
        frame::ProtocolNumber protocol_number;
        Address remote;
        frame::FrameBufferReader reader;
    };
} // namespace net::link
