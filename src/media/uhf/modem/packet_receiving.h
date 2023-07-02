#pragma once

#include "./error.h"
#include "./generic.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/tuple.h>

namespace media::uhf::modem {
    class PacketReceivingCommand : public nb::stream::EmptyStreamReader {};

    template <typename Packet>
    class PacketReceivingResponseBody {
        static_assert(nb::stream::is_stream_writer_v<Packet>);

        nb::stream::TinyByteWriter<2> length_;
        Packet packet_;

      public:
        PacketReceivingResponseBody() = delete;

        template <typename... Ts>
        PacketReceivingResponseBody(Ts &&...ts) : packet_{etl::forward<Ts>(ts)...} {};

        inline constexpr nb::stream::TupleStreamWriter<nb::stream::TinyByteWriter<2> &, Packet &>
        delegate() {
            return {length_, packet_};
        }

        inline constexpr nb::stream::
            TupleStreamWriter<const nb::stream::TinyByteWriter<2> &, const Packet &>
            delegate() const {
            return {length_, packet_};
        }

        inline constexpr Packet &get_packet() {
            return packet_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<nb::stream::StreamWriterDelegate<
                      PacketReceivingResponseBody<nb::stream::EmptyStreamWriter>>>);

    template <typename Packet>
    class PacketReceivingResponse
        : public Response<nb::stream::StreamWriterDelegate<PacketReceivingResponseBody<Packet>>> {
      public:
        PacketReceivingResponse() = delete;

        template <typename... Ts>
        PacketReceivingResponse(Ts &&...ts)
            : Response<PacketReceivingResponseBody<Packet>>{etl::forward<Ts>(ts)...} {};

        inline constexpr Packet &get_packet() {
            return this->get_body().get_packet();
        }
    };
} // namespace media::uhf::modem
