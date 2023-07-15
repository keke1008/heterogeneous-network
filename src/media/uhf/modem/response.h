#pragma once

#include "./error.h"
#include <nb/future.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    enum class ResponseName : uint8_t {
        CarrierSense,
        SerialNumber,
        DataTransmission,
        EquipmentId,
        DestinationId,
        DataReceived,
        Error,
        InformationResponse,
    };

    etl::optional<ResponseName>
    deserialize_response_name(const collection::TinyBuffer<uint8_t, 2> &bytes);

    class ResponseHeader {
        nb::stream::DiscardingStreamWriter prefix_{1};
        nb::stream::TinyByteWriter<2> response_name_;
        nb::stream::DiscardingStreamWriter equal_sign{1};

      public:
        inline constexpr const nb::stream::TupleStreamWriter<
            const nb::stream::DiscardingStreamWriter &,
            const nb::stream::TinyByteWriter<2> &,
            const nb::stream::DiscardingStreamWriter &>
        delegate_writer() const {
            return {prefix_, response_name_, equal_sign};
        }

        inline constexpr nb::stream::TupleStreamWriter<
            nb::stream::DiscardingStreamWriter &,
            nb::stream::TinyByteWriter<2> &,
            nb::stream::DiscardingStreamWriter &>
        delegate_writer() {
            return {prefix_, response_name_, equal_sign};
        }

        nb::Poll<ModemResult<ResponseName>> poll() {
            auto &&name_bytes = POLL_UNWRAP_OR_RETURN(response_name_.poll());
            auto name = deserialize_response_name(name_bytes);
            if (name.has_value()) {
                return nb::Ok{name.value()};
            }
            return nb::Err{ModemError::invalid_response_name(name_bytes)};
        }
    };

    static_assert(nb::stream::is_stream_writer_v<nb::stream::StreamWriterDelegate<ResponseHeader>>);
    static_assert(nb::is_future_v<ResponseHeader, ModemResult<ResponseName>>);

    template <typename ResponseBody>
    class Response {
        static_assert(nb::stream::is_stream_writer_v<ResponseBody>);

        ResponseBody body_;
        nb::stream::DiscardingStreamWriter terminator_{2};

      public:
        Response() = default;

        template <typename... Ts>
        Response(Ts &&...args) : body_{etl::forward<Ts>(args)...} {}

        inline constexpr const nb::stream::
            TupleStreamWriter<const ResponseBody &, const nb::stream::DiscardingStreamWriter &>
            delegate_writer() const {
            return {body_, terminator_};
        }

        inline constexpr nb::stream::
            TupleStreamWriter<ResponseBody &, nb::stream::DiscardingStreamWriter &>
            delegate_writer() {
            return {body_, terminator_};
        }

        inline const ResponseBody &get_body() const {
            return body_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<
                  nb::stream::StreamWriterDelegate<Response<nb::stream::EmptyStreamWriter>>>);

} // namespace media::uhf::modem
