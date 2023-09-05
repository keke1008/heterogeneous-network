#pragma once

#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/hex.h>

namespace media::uhf {
    template <typename Serial>
    class ResponseReader {
        memory::Reference<Serial> serial_;
        uint8_t total_length_;

        Serial &serial() {
            return serial_.get().value().get();
        }

        inline constexpr const Serial &serial() const {
            return serial_.get().value().get();
        }

      public:
        ResponseReader(memory::Reference<Serial> &&serial, uint8_t total_length)
            : serial_{etl::move(serial)},
              total_length_{total_length} {}

        inline constexpr bool is_readable() const {
            return serial().is_readable();
        }

        inline constexpr auto readable_count() const {
            return serial().readable_count();
        }

        inline constexpr etl::optional<uint8_t> read() {
            return serial().read();
        }

        inline constexpr bool is_reader_closed() const {
            return serial().is_reader_closed();
        }

        inline constexpr void close() {
            serial_.unpair();
        }

        inline constexpr uint8_t total_length() const {
            return total_length_;
        }
    };

    template <typename Serial>
    class DRExecutor {
        nb::stream::TinyByteWriter<4> prefix_;
        nb::stream::TinyByteWriter<2> length_;
        nb::Promise<ResponseReader<Serial>> body_;
        nb::stream::TinyByteWriter<2> suffix_;

      public:
        DRExecutor(nb::Promise<ResponseReader<Serial>> &&body) : body_{etl::move(body)} {}

        nb::Poll<nb::Empty> poll(memory::Owned<Serial> &serial) {
            if (!length_.is_writer_closed()) {
                nb::stream::pipe_writers(serial.get(), prefix_, length_);
                if (!length_.is_writer_closed()) {
                    return nb::pending;
                }

                auto raw_length = POLL_UNWRAP_OR_RETURN(length_.poll());
                auto length = serde::hex::deserialize<uint8_t>(raw_length).value();
                auto serial_ref = etl::move(serial.try_create_pair().value());
                ResponseReader<Serial> reader{etl::move(serial_ref), length};
                body_.set_value(etl::move(reader));
            }

            if (serial.has_pair()) {
                return nb::pending;
            }

            if (!suffix_.is_writer_closed()) {
                nb::stream::pipe_writers(serial.get(), suffix_);
                if (!suffix_.is_writer_closed()) {
                    return nb::pending;
                }
            }

            return nb::Ready{nb::Empty{}};
        }
    };
} // namespace media::uhf
