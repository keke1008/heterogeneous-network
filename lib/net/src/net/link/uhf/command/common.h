#pragma once

#include <etl/functional.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <stdint.h>

namespace net::link::uhf {
    template <uint8_t ResponseBodySize>
    class FixedResponseWriter {
        nb::stream::TinyByteWriter<4> prefix_;
        nb::stream::TinyByteWriter<ResponseBodySize> body_;
        nb::stream::TinyByteWriter<2> suffix_;

      public:
        inline constexpr bool is_writable() const {
            return suffix_.is_writable();
        }

        inline constexpr uint8_t writable_count() const {
            return nb::stream::writable_count(prefix_, body_, suffix_);
        }

        inline constexpr bool write(uint8_t byte) {
            return nb::stream::write(byte, prefix_, body_, suffix_);
        }

        inline constexpr bool is_writer_closed() const {
            return suffix_.is_writer_closed();
        }

        inline constexpr bool has_written() const {
            return prefix_.writable_count() != 4;
        }

        nb::Poll<etl::reference_wrapper<const collection::TinyBuffer<uint8_t, ResponseBodySize>>>
        poll() {
            if (!suffix_.is_writer_closed()) {
                return nb::pending;
            }
            return body_.poll();
        }
    };

    template <uint8_t CommandBodySize, uint8_t ResponseBodySize>
    class FixedExecutor {
        nb::stream::TinyByteReader<3 + CommandBodySize + 2> command_;
        FixedResponseWriter<ResponseBodySize> response_;

      public:
        template <typename... CommandBytes>
        FixedExecutor(CommandBytes... command) : command_{command...} {}

        template <typename Serial>
        nb::Poll<etl::reference_wrapper<const collection::TinyBuffer<uint8_t, ResponseBodySize>>>
        poll(Serial &serial) {
            if (!command_.is_reader_closed()) {
                nb::stream::pipe(command_, serial);
                return nb::pending;
            }

            nb::stream::pipe(serial, response_);
            return response_.poll();
        }
    };
} // namespace net::link::uhf
