#pragma once

#include "../../link/address.h"
#include "../protocol_number.h"
#include "nb/poll.h"
#include "nb/stream.h"
#include "nb/stream/tiny_bytes.h"
#include "serde/bytes.h"
#include <stdint.h>

namespace net::protocol::transmission {
    struct PrePacket {
        uint32_t data_octet_count_;
        uint16_t fragment_count_;
        link::Address source_;
        link::Address destination_;

        inline constexpr PrePacket(
            uint32_t data_octet_count,
            uint16_t fragment_count,
            link::Address &source,
            link::Address &destination
        )
            : data_octet_count_{data_octet_count},
              fragment_count_{fragment_count},
              source_{source},
              destination_{destination} {}
    };

    class PrePacketDeserializer {
        nb::stream::TinyByteWriter<sizeof(uint32_t)> data_octet_count_;
        nb::stream::TinyByteWriter<sizeof(uint16_t)> fragment_count_;
        link::AddressDeserializer source_;
        link::AddressDeserializer destination_;

      public:
        template <typename Reader>
        nb::Poll<PrePacket> poll(Reader &reader) {
            nb::stream::pipe(reader, data_octet_count_);
            auto data_octet_count = POLL_UNWRAP_OR_RETURN(data_octet_count_.poll());

            nb::stream::pipe(reader, fragment_count_);
            auto fragment_count = POLL_UNWRAP_OR_RETURN(fragment_count_.poll());

            auto source = POLL_UNWRAP_OR_RETURN(source_.poll(reader));
            auto destination = POLL_UNWRAP_OR_RETURN(destination_.poll(reader));
            return PrePacket{
                serde::bytes::deserialize<uint32_t>(data_octet_count.get()),
                serde::bytes::deserialize<uint16_t>(fragment_count.get()),
                source,
                destination,
            };
        }
    };

    class PrePacketSerializer {
        nb::stream::TinyByteReader<1> protocol_number_{protocol_number::TRANSMISSION_PRE};
        nb::stream::TinyByteReader<sizeof(uint32_t)> data_octet_count_;
        nb::stream::TinyByteReader<sizeof(uint16_t)> fragment_count_;
        link::AddressSerializer source_;
        link::AddressSerializer destination_;

      public:
        template <typename Writer>
        nb::Poll<nb::Empty> poll(Writer &writer) {
            nb::stream::pipe_readers(writer, protocol_number_, data_octet_count_, fragment_count_);
            if (!fragment_count_.is_reader_closed()) {
                return nb::pending;
            }

            POLL_UNWRAP_OR_RETURN(source_.poll(writer));
            POLL_UNWRAP_OR_RETURN(destination_.poll(writer));
            return nb::Poll<nb::Empty>{nb::Empty{}};
        }
    };
} // namespace net::protocol::transmission
