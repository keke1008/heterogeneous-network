#pragma once

#include "./command.h"
#include "./error.h"
#include "./response.h"
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
    class PacketTransmissionCommand {
        Command<nb::stream::StreamReaderDelegate<PacketTransmissionCommandBody<Packet>>> command_;

      public:
        template <typename... Ts>
        inline constexpr PacketTransmissionCommand(Ts &&...ts)
            : command_{CommandName::DataTransmission, etl::forward<Ts>(ts)...} {}

        inline constexpr decltype(auto) delegate() {
            return command_.delegate();
        }

        inline constexpr decltype(auto) delegate() const {
            return command_.delegate();
        }
    };

    class PacketTransmissionResponse {
        Response<nb::stream::TinyByteWriter<2>> response_;

      public:
        PacketTransmissionResponse() = default;

        inline constexpr decltype(auto) delegate() {
            return response_.delegate();
        }

        inline constexpr decltype(auto) delegate() const {
            return response_.delegate();
        }

        nb::Poll<const collection::TinyBuffer<uint8_t, 2> &&> poll() {
            return response_.get_body().poll();
        }
    };

    static_assert(nb::is_future_v<
                  PacketTransmissionResponse,
                  const collection::TinyBuffer<uint8_t, 2> &&>);
} // namespace media::uhf::modem
