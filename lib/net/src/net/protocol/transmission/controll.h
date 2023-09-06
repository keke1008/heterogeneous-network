#pragma once

#include "../../link/address.h"
#include "../protocol_number.h"
#include "nb/poll.h"
#include "new.h"

namespace net::protocol::transmission {
    struct ControlPacket {
        link::Address source_;
        link::Address destination_;

      public:
        inline constexpr ControlPacket(link::Address &source, link::Address &destination)
            : source_{source},
              destination_{destination} {}
    };

    class ControlPacketDeserializer {
        link::AddressDeserializer source_;
        link::AddressDeserializer destination_;

      public:
        template <typename Reader>
        nb::Poll<ControlPacket> poll(Reader &reader) {
            auto source = POLL_UNWRAP_OR_RETURN(source_.poll(reader));
            auto destination = POLL_UNWRAP_OR_RETURN(destination_.poll(reader));
            return ControlPacket{source, destination};
        }
    };

    template <uint8_t PROTOCOL_NUMBER>
    class ControlPacketSerializer {
        nb::stream::TinyByteReader<1> protocol_number_{PROTOCOL_NUMBER};
        link::AddressSerializer source_;
        link::AddressSerializer destination_;

      public:
        inline constexpr ControlPacketSerializer(ControlPacket &packet)
            : source_{packet.source_},
              destination_{packet.destination_} {}

        template <typename Writer>
        nb::Poll<nb::Empty> poll(Writer &writer) {
            nb::stream::pipe(protocol_number_, writer);
            POLL_UNWRAP_OR_RETURN(source_.poll(writer));
            POLL_UNWRAP_OR_RETURN(destination_.poll(writer));
            return nb::empty;
        }
    };

    struct AckPacket : public ControlPacket {
        using ControlPacket::ControlPacket;
    };

    class AckPacketDeserializer : public ControlPacketDeserializer {
        using ControlPacketDeserializer::ControlPacketDeserializer;
    };

    class AckPacketSerializer : public ControlPacketSerializer<protocol_number::TRANSMISSION_ACK> {
        using ControlPacketSerializer<protocol_number::TRANSMISSION_ACK>::ControlPacketSerializer;
    };

    struct NackPacket : public ControlPacket {
        using ControlPacket::ControlPacket;
    };

    class NackPacketDeserializer : public ControlPacketDeserializer {
        using ControlPacketDeserializer::ControlPacketDeserializer;
    };

    class NackPacketSerializer
        : public ControlPacketSerializer<protocol_number::TRANSMISSION_NACK> {
        using ControlPacketSerializer<protocol_number::TRANSMISSION_NACK>::ControlPacketSerializer;
    };
} // namespace net::protocol::transmission
