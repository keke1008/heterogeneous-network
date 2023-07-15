#pragma once

#include "./template.h"
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace media::uhf::modem {
    template <typename Data>
    class DataTransmissionCommandBody {
        static_assert(nb::stream::is_stream_reader_v<Data>);

        nb::stream::TinyByteReader<2> length_;
        Data data_;

      public:
        DataTransmissionCommandBody() = delete;

        template <typename... Ts>
        DataTransmissionCommandBody(Ts &&...ts) : data_{etl::forward<Ts>(ts)...} {
            length_ = serde::hex::serialize<uint8_t>(data_.readable_count());
        }

        inline constexpr nb::stream::TupleStreamReader<nb::stream::TinyByteReader<2> &, Data &>
        delegate_reader() {
            return {length_, data_};
        }

        inline constexpr nb::stream::
            TupleStreamReader<const nb::stream::TinyByteReader<2> &, const Data &>
            delegate_reader() const {
            return {length_, data_};
        }
    };

    template <typename Data>
    class DataTransmissionCommand {
        Command<nb::stream::StreamReaderDelegate<DataTransmissionCommandBody<Data>>> command_;

      public:
        template <typename... Ts>
        inline constexpr DataTransmissionCommand(Ts &&...ts)
            : command_{CommandName::DataTransmission, etl::forward<Ts>(ts)...} {}

        inline constexpr decltype(auto) delegate_reader() {
            return command_.delegate_reader();
        }

        inline constexpr decltype(auto) delegate_reader() const {
            return command_.delegate_reader();
        }
    };

    class DataTransmissionResponse {
        Response<nb::stream::TinyByteWriter<2>> response_;

      public:
        DataTransmissionResponse() = default;

        inline constexpr decltype(auto) delegate_writer() {
            return response_.delegate_writer();
        }

        inline constexpr decltype(auto) delegate_writer() const {
            return response_.delegate_writer();
        }

        nb::Poll<const collection::TinyBuffer<uint8_t, 2> &&> poll() {
            return response_.get_body().poll();
        }
    };

    static_assert(nb::is_future_v<
                  DataTransmissionResponse,
                  const collection::TinyBuffer<uint8_t, 2> &&>);

    template <typename Data>
    using DataTransmissionTask = Task<DataTransmissionCommand<Data>, DataTransmissionResponse>;

} // namespace media::uhf::modem
