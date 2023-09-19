#pragma once

#include <etl/functional.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <stdint.h>

namespace net::link::uhf {
    template <uint8_t ResponseBodySize>
    class FixedResponseWriter final : public nb::stream::WritableBuffer {
        nb::stream::FixedWritableBuffer<4> prefix_;
        nb::stream::FixedWritableBuffer<ResponseBodySize> body_;
        nb::stream::FixedWritableBuffer<2> suffix_;

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return nb::stream::write_all_from(source, prefix_, body_, suffix_);
        }

        nb::Poll<etl::span<uint8_t>> poll() {
            POLL_UNWRAP_OR_RETURN(suffix_.poll());
            return body_.poll();
        }

        inline bool has_written() const {
            return prefix_.writable_count() != 4;
        }
    };

    template <uint8_t CommandBodySize, uint8_t ResponseBodySize>
    class FixedExecutor {
        nb::stream::FixedReadableBuffer<3 + CommandBodySize + 2> command_;
        FixedResponseWriter<ResponseBodySize> response_;

      public:
        template <typename... CommandBytes>
        FixedExecutor(CommandBytes... command) : command_{command...} {}

        nb::Poll<etl::span<uint8_t>> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
            return response_.poll();
        }
    };
} // namespace net::link::uhf
