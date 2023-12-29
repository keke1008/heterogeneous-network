#pragma once

#include "../address.h"
#include "../media.h"
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/span.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <util/span.h>

namespace net::link::wifi {
    class WifiIpV4Address {
        etl::array<uint8_t, 4> address_;

      public:
        explicit WifiIpV4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            : address_{a, b, c, d} {}

        explicit WifiIpV4Address(etl::span<const uint8_t> address) {
            FASSERT(address.size() == 4);
            etl::copy(address.begin(), address.end(), address_.data());
        }

        explicit WifiIpV4Address(const Address &address) {
            FASSERT(address.type() == AddressType::IPv4);
            FASSERT(address.body().size() == 6);
            address_.assign(address.body().begin(), address.body().begin() + 4);
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
        nb::ser::Dec<uint8_t> octet1_;
        nb::ser::Bin<uint8_t> dot1_{'.'};
        nb::ser::Dec<uint8_t> octet2_;
        nb::ser::Bin<uint8_t> dot2_{'.'};
        nb::ser::Dec<uint8_t> octet3_;
        nb::ser::Bin<uint8_t> dot3_{'.'};
        nb::ser::Dec<uint8_t> octet4_;

      public:
        explicit AsyncWifiIpV4AddressSerializer(const WifiIpV4Address &address)
            : octet1_(address.address()[0]),
              octet2_(address.address()[1]),
              octet3_(address.address()[2]),
              octet4_(address.address()[3]) {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(octet1_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(dot1_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(octet2_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(dot2_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(octet3_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(dot3_.serialize(writer));
            return octet4_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return octet1_.serialized_length() + dot1_.serialized_length() +
                octet2_.serialized_length() + dot2_.serialized_length() +
                octet3_.serialized_length() + dot3_.serialized_length() +
                octet4_.serialized_length();
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
            FASSERT(address.type() == AddressType::IPv4);
            FASSERT(address.body().size() == 6);
            port_ =
                nb::deserialize_span_at_once(address.body().subspan(4, 2), nb::de::Bin<uint16_t>{});
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

      public:
        etl::array<uint8_t, 6> into_array() const {
            etl::array<uint8_t, 6> addr{};
            auto ipaddr = ip_.address();
            addr.assign(ipaddr.begin(), ipaddr.end());

            auto result = nb::serialize_span(
                etl::span{addr}.subspan(4, 2), nb::ser::Bin<uint16_t>{port_.port()}
            );
            FASSERT(result.is_ready() && result.unwrap() == nb::SerializeResult::Ok);

            return addr;
        }

        explicit WifiAddress(WifiIpV4Address ip, WifiPort port) : ip_{ip}, port_{port} {}

        explicit WifiAddress(const Address &address) : ip_{address}, port_{address} {}

        explicit WifiAddress(const LinkAddress &address)
            : WifiAddress{([&]() {
                  FASSERT(address.is_unicast());
                  return address.unwrap_unicast().address;
              })()} {}

        explicit operator Address() const {
            return Address{AddressType::IPv4, into_array()};
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

    class AsyncWifiAddressBinarySerializer {
        nb::ser::Array<nb::ser::Bin<uint8_t>, 6> address_;

      public:
        explicit AsyncWifiAddressBinarySerializer(const WifiAddress &address)
            : address_{address.into_array()} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return address_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return address_.serialized_length();
        }
    };

    class AsyncWifiAddressBinaryDeserializer {
        nb::de::Array<nb::de::Bin<uint8_t>, 6> address_{6};

      public:
        inline WifiAddress result() const {
            const auto &result = address_.result();
            auto span = etl::span<const uint8_t, 6>{result};
            return WifiAddress{
                WifiIpV4Address{span.subspan(0, 4)},
                WifiPort{nb::deserialize_span_at_once(span.subspan(4, 2), nb::de::Bin<uint16_t>{})}
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return address_.deserialize(reader);
        }
    };

    static constexpr uint8_t WIFI_FRAME_TYPE_SIZE = 1;

    enum class WifiFrameType : uint8_t {
        Data = 1,
        GlobalAddressRequest = 2,
        GlobalAddressResponse = 3,
    };

    class AsyncWifiFrameTypeSerializer {
        nb::ser::Bin<uint8_t> frame_type_;

      public:
        explicit AsyncWifiFrameTypeSerializer(WifiFrameType frame_type)
            : frame_type_{static_cast<uint8_t>(frame_type)} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return frame_type_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return frame_type_.serialized_length();
        }
    };

    class AsyncWifiFrameTypeDeserializer {
        nb::de::Bin<uint8_t> frame_type_;

      public:
        inline WifiFrameType result() const {
            return static_cast<WifiFrameType>(frame_type_.result());
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(frame_type_.deserialize(reader));
            auto result = frame_type_.result();
            switch (result) {
            case static_cast<uint8_t>(WifiFrameType::Data):
            case static_cast<uint8_t>(WifiFrameType::GlobalAddressRequest):
            case static_cast<uint8_t>(WifiFrameType::GlobalAddressResponse):
                return nb::de::DeserializeResult::Ok;
            default:
                return nb::de::DeserializeResult::Invalid;
            }
        }
    };

    struct WifiGlobalAddressRequestFrame {
        static inline constexpr WifiFrameType frame_type() {
            return WifiFrameType::GlobalAddressRequest;
        }

        inline constexpr uint8_t body_length() const {
            return WIFI_FRAME_TYPE_SIZE;
        }
    };

    struct WifiGlobalAddressResponseFrame {
        WifiAddress address;

        static inline constexpr WifiFrameType frame_type() {
            return WifiFrameType::GlobalAddressResponse;
        }

        inline uint8_t body_length() const {
            return AsyncWifiAddressBinarySerializer{address}.serialized_length() +
                WIFI_FRAME_TYPE_SIZE;
        }
    };

    using WifiControlFrame =
        etl::variant<WifiGlobalAddressRequestFrame, WifiGlobalAddressResponseFrame>;

    class AsyncWifiControlFrameSerializer {
        using Serializer = nb::ser::Union<nb::ser::Empty, AsyncWifiAddressBinarySerializer>;

        AsyncWifiFrameTypeSerializer frame_type_;
        Serializer address_;

        static Serializer address(const WifiControlFrame &frame) {
            return etl::visit<Serializer>(
                util::Visitor{
                    [](const WifiGlobalAddressRequestFrame &) {
                        return Serializer{nb::ser::Empty{}};
                    },
                    [](const WifiGlobalAddressResponseFrame &frame) {
                        return Serializer{AsyncWifiAddressBinarySerializer{frame.address}};
                    },
                },
                frame
            );
        }

      public:
        explicit AsyncWifiControlFrameSerializer(WifiControlFrame &&frame)
            : frame_type_{etl::visit([](auto &f) { return f.frame_type(); }, frame)},
              address_{address(frame)} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(writer));
            return address_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return frame_type_.serialized_length() + address_.serialized_length();
        }
    };

    struct WifiDataFrame {
        frame::ProtocolNumber protocol_number;
        WifiAddress remote;
        frame::FrameBufferReader reader;

        static WifiDataFrame from_link_frame(LinkFrame &&frame) {
            return WifiDataFrame{
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

        inline uint8_t body_length() const {
            return frame::PROTOCOL_SIZE + WIFI_FRAME_TYPE_SIZE + reader.buffer_length();
        }
    };

    class AsyncWifiDataFrameSerializer {
        AsyncWifiFrameTypeSerializer frame_type_{WifiFrameType::Data};
        frame::AsyncProtocolNumberSerializer protocol_;
        frame::AsyncFrameBufferReaderSerializer reader_;

      public:
        explicit AsyncWifiDataFrameSerializer(WifiDataFrame &&frame)
            : protocol_{frame.protocol_number},
              reader_{etl::move(frame.reader)} {}

        template <nb::AsyncReadable R>
        nb::Poll<nb::SerializeResult> serialize(R &reader) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(protocol_.serialize(reader));
            return reader_.serialize(reader);
        }

        uint8_t serialized_length() const {
            return protocol_.serialized_length() + reader_.serialized_length();
        }
    };

    using WifiFrame =
        etl::variant<WifiGlobalAddressRequestFrame, WifiGlobalAddressResponseFrame, WifiDataFrame>;
} // namespace net::link::wifi
