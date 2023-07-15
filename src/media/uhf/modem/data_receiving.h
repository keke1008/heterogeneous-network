#pragma once

#include "./template.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/tuple.h>

namespace media::uhf::modem {
    class DataReceivingCommand {
        nb::stream::EmptyStreamReader reader_;

      public:
        inline constexpr decltype(auto) delegate_reader() {
            return reader_;
        }

        inline constexpr decltype(auto) delegate_reader() const {
            return reader_;
        }
    };

    template <typename Data>
    class DataReceivingResponseBody {
        static_assert(nb::stream::is_stream_writer_v<Data>);

        nb::stream::TinyByteWriter<2> length_;
        Data data_;

      public:
        DataReceivingResponseBody() = default;

        template <typename... Ts>
        DataReceivingResponseBody(Ts &&...ts) : data_{etl::forward<Ts>(ts)...} {};

        inline constexpr nb::stream::TupleStreamWriter<nb::stream::TinyByteWriter<2> &, Data &>
        delegate_writer() {
            return {length_, data_};
        }

        inline constexpr nb::stream::
            TupleStreamWriter<const nb::stream::TinyByteWriter<2> &, const Data &>
            delegate_writer() const {
            return {length_, data_};
        }

        inline constexpr const Data &get_data() const {
            return data_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<nb::stream::StreamWriterDelegate<
                      DataReceivingResponseBody<nb::stream::EmptyStreamWriter>>>);

    template <typename Data>
    class DataReceivingResponse
        : public Response<nb::stream::StreamWriterDelegate<DataReceivingResponseBody<Data>>> {
      public:
        DataReceivingResponse() = default;

        template <typename... Ts>
        DataReceivingResponse(Ts &&...ts)
            : Response<DataReceivingResponseBody<Data>>{etl::forward<Ts>(ts)...} {};

        inline constexpr const Data &get_data() const {
            return this->get_body().get_writer().get_data();
        }
    };

    template <typename Data>
    using DataReceivingTask = Task<DataReceivingCommand, DataReceivingResponse<Data>>;
} // namespace media::uhf::modem
