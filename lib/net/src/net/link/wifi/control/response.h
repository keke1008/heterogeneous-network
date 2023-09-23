#pragma once

#include <etl/array.h>
#include <etl/span.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <stdint.h>
#include <util/progmem.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

namespace net::link::wifi {
    enum class ResponseType : uint8_t {
        NEVER,
        OK,
        ERROR,
        FAIL,
        SEND_OK,
    };

    namespace {
        inline constexpr const char *as_string(ResponseType type) {
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
            };
        }

        inline constexpr uint8_t length(ResponseType type) {
            return etl::char_traits<char>::length(as_string(type));
        }

        template <ResponseType T>
        inline etl::span<const uint8_t> as_span() {
            constexpr const char *str = as_string(T);
            constexpr const uint8_t len = length(T);
            return {reinterpret_cast<const uint8_t *>(str), len};
        }
    } // namespace

    template <ResponseType SUCCESS_RESPONSE, ResponseType FAILURE_RESPONSE>
    class ExactMatchResponse final : public nb::stream::WritableBuffer {
        static constexpr uint8_t MAX_LINE_LENGTH =
            etl::max(length(SUCCESS_RESPONSE), length(FAILURE_RESPONSE));

        nb::stream::MaxLengthSingleLineWrtableBuffer<MAX_LINE_LENGTH> buffer_;
        nb::Promise<bool> promise_;

        etl::span<const uint8_t> success_response_{as_span<SUCCESS_RESPONSE>()};
        etl::span<const uint8_t> failure_response_{as_span<FAILURE_RESPONSE>()};

      public:
        explicit ExactMatchResponse(nb::Promise<bool> promise) : promise_{etl::move(promise)} {}

        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            while (true) {
                POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(source));

                auto written_bytes = buffer_.poll().unwrap();
                if (etl::equal(written_bytes, success_response_)) {
                    promise_.set_value(true);
                    return nb::ready();
                }
                if (etl::equal(written_bytes, failure_response_)) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                buffer_.reset();
            }
        }
    };
} // namespace net::link::wifi
