#pragma once

#include "../address/udp.h"
#include <etl/algorithm.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <net/link.h>
#include <util/span.h>

namespace media::wifi {
    class AsyncIpV4AddressSerializer {
        nb::ser::Dec<uint8_t> octet1_;
        nb::ser::Bin<uint8_t> dot1_{'.'};
        nb::ser::Dec<uint8_t> octet2_;
        nb::ser::Bin<uint8_t> dot2_{'.'};
        nb::ser::Dec<uint8_t> octet3_;
        nb::ser::Bin<uint8_t> dot3_{'.'};
        nb::ser::Dec<uint8_t> octet4_;

      public:
        explicit AsyncIpV4AddressSerializer(const UdpIpAddress &address)
            : octet1_(address.body()[0]),
              octet2_(address.body()[1]),
              octet3_(address.body()[2]),
              octet4_(address.body()[3]) {}

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

    class AsyncUdpIpAddressDeserializer {
        nb::de::Array<nb::de::Dec<uint8_t>, 4> address_{4};

      public:
        inline UdpIpAddress result() const {
            return UdpIpAddress{address_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return address_.deserialize(reader);
        }
    };

    class AsyncUdpPortSerializer {
        nb::ser::Dec<uint16_t> port_;

      public:
        explicit AsyncUdpPortSerializer(UdpPort port) : port_{port.value()} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return port_.serialize(writer);
        }

        constexpr uint8_t serialized_length() const {
            return port_.serialized_length();
        }
    };

    class AsyncTransportPortDeserializer {
        nb::de::Dec<uint16_t> port_;

      public:
        inline UdpPort result() const {
            return UdpPort{port_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return port_.deserialize(reader);
        }
    };

    class AsyncUdpAddressDeserializer {
        AsyncUdpIpAddressDeserializer ip_;
        AsyncTransportPortDeserializer port_;

      public:
        inline UdpAddress result() const {
            return UdpAddress{ip_.result(), port_.result()};
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(ip_.deserialize(reader));
            return port_.deserialize(reader);
        }
    };

    struct WifiFrame {
        net::frame::ProtocolNumber protocol_number;
        UdpAddress remote;
        net::frame::FrameBufferReader reader;

        static WifiFrame from_link_frame(net::link::LinkFrame &&frame) {
            return WifiFrame{
                .protocol_number = frame.protocol_number,
                .remote = UdpAddress{frame.remote},
                .reader = etl::move(frame.reader),
            };
        }

        inline uint8_t body_length() const {
            return net::frame::PROTOCOL_SIZE + reader.buffer_length();
        }
    };

    class AsyncWifiFrameSerializer {
        net::frame::AsyncProtocolNumberSerializer protocol_;
        net::frame::AsyncFrameBufferReaderSerializer reader_;

      public:
        explicit AsyncWifiFrameSerializer(WifiFrame &&frame)
            : protocol_{frame.protocol_number},
              reader_{etl::move(frame.reader)} {}

        template <nb::AsyncReadable R>
        nb::Poll<nb::SerializeResult> serialize(R &reader) {
            SERDE_SERIALIZE_OR_RETURN(protocol_.serialize(reader));
            return reader_.serialize(reader);
        }

        uint8_t serialized_length() const {
            return protocol_.serialized_length() + reader_.serialized_length();
        }
    };
} // namespace media::wifi
