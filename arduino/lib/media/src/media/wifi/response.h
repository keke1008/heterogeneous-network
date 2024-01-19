#pragma once

#include <etl/span.h>
#include <etl/string_view.h>
#include <logger.h>
#include <nb/serde.h>
#include <util/span.h>

namespace media::wifi {
    enum class ResponseType : uint8_t {
        NEVER,
        OK,
        ERROR,
        FAIL,
        SEND_OK,
        SEND_FAIL,
    };

    inline constexpr etl::string_view as_string_view(ResponseType type) {
        switch (type) {
        case ResponseType::NEVER:
            return "ERROR\r\n";
        case ResponseType::OK:
            return "OK\r\n";
        case ResponseType::ERROR:
            return "ERROR\r\n";
        case ResponseType::FAIL:
            return "FAIL\r\n";
        case ResponseType::SEND_OK:
            return "SEND OK\r\n";
        case ResponseType::SEND_FAIL:
            return "SEND FAIL\r\n";
        default:
            UNREACHABLE_DEFAULT_CASE;
        };
    }

    inline constexpr bool is_success_response(ResponseType type) {
        switch (type) {
        case ResponseType::OK:
        case ResponseType::SEND_OK:
            return true;
        default:
            return false;
        }
    }

    inline constexpr bool is_error_response(ResponseType type) {
        return !is_success_response(type);
    }

    template <ResponseType T>
    constexpr uint8_t response_type_length = as_string_view(T).size();

    class AsyncBufferedResponseTypeDeserializer {
        etl::optional<ResponseType> result_;

      public:
        template <nb::AsyncBufferedReadable R>
        nb::Poll<nb::DeserializeResult> deserialize(R &readable) {
            for (auto type : {
                     ResponseType::OK,
                     ResponseType::FAIL,
                     ResponseType::ERROR,
                     ResponseType::SEND_OK,
                     ResponseType::SEND_FAIL,
                 }) {
                auto template_str = as_string_view(type);
                SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(template_str.size()));

                auto str = util::as_str(readable.read_span_unchecked(template_str.size()));
                if (str == template_str) {
                    result_ = type;
                    return nb::DeserializeResult::Ok;
                }

                readable.seek(0);
            }

            return nb::DeserializeResult::Invalid;
        }

        inline ResponseType result() const {
            return *result_;
        }
    };

    using AsyncResponseTypeBytesDeserializer =
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<11>; // longest: SEND FAIL\r\n

    /**
     * ResponseTypeの行まで読み捨て，ResponseTypeの行をデシリアライズする
     *
     * 読み込み可能なデータがない場合に限り`DeserializeResult::NotEnoughLength`を返す．
     * それ以外の場合は`DeserializeResult::Ok`を返す．
     */
    class AsyncResponseTypeDeserializer {
        AsyncResponseTypeBytesDeserializer response_;
        AsyncBufferedResponseTypeDeserializer buffered_deserializer_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &readable) {
            while (true) {
                auto result = POLL_UNWRAP_OR_RETURN(response_.deserialize(readable));
                if (result != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                auto line = response_.result();
                result = POLL_UNWRAP_OR_RETURN(nb::deserialize_span(line, buffered_deserializer_));
                if (result == nb::DeserializeResult::Ok) {
                    return result;
                }

                response_.reset();
            }
        }

        inline ResponseType result() const {
            return buffered_deserializer_.result();
        }
    };

} // namespace media::wifi
