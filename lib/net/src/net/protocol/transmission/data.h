#pragma once

#include "../../link/address.h"
#include "../../packet.h"
#include "../protocol_number.h"
#include "nb/future.h"
#include "nb/poll.h"
#include "nb/stream.h"
#include <etl/utility.h>
#include <stdint.h>

namespace net::protocol::transmission {
    struct DataPacket {
        uint16_t fragment_offset_;
        uint8_t body_length_;
        link::Address source_;
        link::Address destination_;
        packet::PacketBufferReader data_;

        inline constexpr DataPacket(
            uint16_t fragment_offset,
            uint8_t body_length,
            link::Address &source,
            link::Address &destination,
            packet::PacketBufferReader &&data
        )
            : fragment_offset_{fragment_offset},
              body_length_{body_length},
              source_{source},
              destination_{destination},
              data_{etl::move(data)} {}
    };

    struct DataPacketDeserializeStateHead {
        packet::UninitializedPacketBuffer buffer_;

        DataPacketDeserializeStateHead(packet::UninitializedPacketBuffer &&buffer)
            : buffer_{etl::move(buffer)} {}
    };

    struct DataPacketDeserializeStateBody {
        packet::PacketBufferWriter writer_;

        DataPacketDeserializeStateBody(packet::PacketBufferWriter &&writer)
            : writer_{etl::move(writer)} {}
    };

    class DataPacketDeserializer {
        etl::variant<DataPacketDeserializeStateHead, DataPacketDeserializeStateBody> state_;

        nb::Promise<DataPacket> packet_promise_;

        nb::stream::TinyByteWriter<sizeof(uint16_t)> fragment_offset_;
        nb::stream::TinyByteWriter<sizeof(uint8_t)> body_length_;
        link::AddressDeserializer source_;
        link::AddressDeserializer destination_;

      public:
        DataPacketDeserializer() = delete;

        DataPacketDeserializer(
            packet::UninitializedPacketBuffer &&buffer,
            nb::Promise<DataPacket> &&packet
        )
            : state_{DataPacketDeserializeStateHead{etl::move(buffer)}},
              packet_promise_{etl::move(packet)} {}

        template <typename Reader>
        nb::Poll<nb::Empty> poll(Reader &reader) {
            if (etl::holds_alternative<DataPacketDeserializeStateHead>(state_)) {
                nb::stream::pipe(reader, fragment_offset_);
                auto fragment_offset = POLL_UNWRAP_OR_RETURN(fragment_offset_.poll());

                nb::stream::pipe(reader, body_length_);
                auto body_length = POLL_UNWRAP_OR_RETURN(body_length_.poll());

                auto source = POLL_UNWRAP_OR_RETURN(source_.poll(reader));
                auto destination = POLL_UNWRAP_OR_RETURN(destination_.poll(reader));

                auto body_length_num = serde::bytes::deserialize<uint8_t>(body_length.get());
                auto &buffer = etl::get<DataPacketDeserializeStateHead>(state_).buffer_;
                auto [data_reader, data_writer] = etl::move(buffer).initialize(body_length_num);
                packet_promise_.set_value(DataPacket{
                    serde::bytes::deserialize<uint16_t>(fragment_offset.get()),
                    serde::bytes::deserialize<uint8_t>(body_length.get()),
                    source,
                    destination,
                    etl::move(reader),
                });
                state_ = DataPacketDeserializeStateBody{etl::move(data_writer)};
            }

            auto &data_writer = etl::get<DataPacketDeserializeStateBody>(state_).writer_;
            nb::stream::pipe(reader, data_writer);
            if (!data_writer.is_writer_closed()) {
                return nb::pending;
            }

            return nb::empty;
        }
    };

    class DataPacketSerializer {
        nb::stream::TinyByteReader<1> protocol_number_{protocol_number::TRANSMISSION_DATA};
        nb::stream::TinyByteReader<sizeof(uint16_t)> fragment_offset_;
        nb::stream::TinyByteReader<sizeof(uint8_t)> body_length_;
        link::AddressSerializer source_;
        link::AddressSerializer destination_;

      public:
        DataPacketSerializer() = delete;

        DataPacketSerializer(DataPacket &packet)
            : fragment_offset_{serde::bytes::serialize(packet.fragment_offset_)},
              body_length_{serde::bytes::serialize(packet.body_length_)},
              source_{packet.source_},
              destination_{packet.destination_} {}

        template <typename Writer>
        nb::Poll<nb::Empty> poll(Writer &writer) {
            nb::stream::pipe_readers(writer, protocol_number_, fragment_offset_, body_length_);
            if (!body_length_.is_reader_closed()) {
                return nb::pending;
            }

            POLL_UNWRAP_OR_RETURN(source_.poll(writer));
            POLL_UNWRAP_OR_RETURN(destination_.poll(writer));

            return nb::empty;
        }
    };
} // namespace net::protocol::transmission
