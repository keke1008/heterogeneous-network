#pragma once

#include "./error.h"
#include <nb/future.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    enum class ResponseType : uint8_t {
        CarrierSense,
        GetSerialNumber,
        DataTransmission,
        SetEquipmentId,
        SetDestinationId,
        DataReceiving,
        Error,
        InformationResponse,
    };

    etl::optional<ResponseType>
    deserialize_response_name(const collection::TinyBuffer<uint8_t, 2> &bytes);

    template <typename Reader>
    class ResponseHeader {
        nb::stream::DiscardingStreamWriter prefix_{1};
        nb::stream::TinyByteWriter<2> response_name_;
        nb::stream::DiscardingStreamWriter equal_sign{1};

      public:
        nb::Poll<etl::optional<ResponseType>> read_from(Reader &reader) {
            nb::stream::pipe_writers(reader, prefix_, response_name_, equal_sign);
            auto &&name_bytes = POLL_UNWRAP_OR_RETURN(response_name_.poll());
            return deserialize_response_name(name_bytes);
        }
    };

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

        inline ResponseBody &get_body() {
            return body_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<
                  nb::stream::StreamWriterDelegate<Response<nb::stream::EmptyStreamWriter>>>);

} // namespace media::uhf::modem
