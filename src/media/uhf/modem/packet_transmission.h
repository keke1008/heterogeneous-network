#pragma once

#include "./error.h"
#include "./generic.h"
#include "nb/stream/tuple.h"
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace media::uhf::modem {
    template <typename Packet>
    class PacketTransmissionCommandBody {
        static_assert(nb::stream::is_stream_reader_v<Packet>);

        nb::stream::TinyByteReader<2> length_;
        Packet packet_;

      public:
        PacketTransmissionCommandBody() = delete;

        template <typename... Ts>
        PacketTransmissionCommandBody(Ts &&...ts) : packet_{etl::forward<Ts>(ts)...} {
            length_ = serde::hex::serialize<uint8_t>(packet_.readable_count());
        }

        inline constexpr nb::stream::TupleStreamReader<nb::stream::TinyByteReader<2> &, Packet &>
        delegate() {
            return {length_, packet_};
        }

        inline constexpr nb::stream::
            TupleStreamReader<const nb::stream::TinyByteReader<2> &, const Packet &>
            delegate() const {
            return {length_, packet_};
        }
    };

    template <typename Packet>
    class PacketTransmissionCommand
        : public Command<nb::stream::StreamReaderDelegate<PacketTransmissionCommandBody<Packet>>> {
        static_assert(nb::stream::is_stream_reader_v<Packet>);

      public:
        using Command<
            nb::stream::StreamReaderDelegate<PacketTransmissionCommandBody<Packet>>>::Command;
    };

    class PacketTransmissionResponse : public Response<nb::stream::TinyByteWriter<2>> {
      public:
        using Response<nb::stream::TinyByteWriter<2>>::Response;

        nb::Poll<const collection::TinyBuffer<uint8_t, 2> &&> poll() {
            return get_body().poll();
        }
    };

    static_assert(nb::is_future_v<
                  PacketTransmissionResponse,
                  const collection::TinyBuffer<uint8_t, 2> &&>);
} // namespace media::uhf::modem
