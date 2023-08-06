#pragma once

#include "../common.h"
#include "./common.h"
#include "serde/hex.h"
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>

namespace media::uhf {
    template <typename Serial>
    class CommandWriter {
        memory::Reference<Serial> serial_;

        Serial &serial() {
            return serial_.get().value().get();
        }

        const Serial &serial() const {
            return serial_.get().value().get();
        }

      public:
        CommandWriter(memory::Reference<Serial> &&serial) : serial_{etl::move(serial)} {}

        inline bool is_writable() const {
            return serial().is_writable();
        }

        inline auto writable_count() const {
            return serial().writable_count();
        }

        inline bool write(uint8_t byte) {
            return serial().write(byte);
        }

        inline bool is_writer_closed() const {
            return serial().is_writer_closed();
        }

        inline void close() {
            serial_.unpair();
        }
    };

    template <typename Serial>
    class DTCommand {
        nb::stream::TinyByteReader<3> prefix_{'@', 'D', 'T'};
        nb::stream::TinyByteReader<2> length_;
        nb::Promise<CommandWriter<Serial>> body_;
        nb::stream::TinyByteReader<2> route_prefix_{'/', 'R'};
        nb::stream::TinyByteReader<2> route_;
        nb::stream::TinyByteReader<2> suffix_{'\r', '\n'};

      public:
        explicit DTCommand(ModemId dest, uint8_t length, nb::Promise<CommandWriter<Serial>> body)
            : length_{serde::hex::serialize(length)},
              body_{etl::move(body)},
              route_{dest.serialize()} {}

        nb::Poll<nb::Empty> poll(memory::Owned<Serial> &serial) {
            if (!length_.is_reader_closed()) {
                nb::stream::pipe_readers(serial.get(), prefix_, length_);
                if (!length_.is_reader_closed()) {
                    return nb::pending;
                }

                CommandWriter<Serial> writer{etl::move(serial.try_create_pair().value())};
                body_.set_value(etl::move(writer));
            }

            if (serial.has_pair()) {
                return nb::pending;
            }

            if (!suffix_.is_reader_closed()) {
                nb::stream::pipe_readers(serial.get(), route_prefix_, route_, suffix_);
                if (!suffix_.is_reader_closed()) {
                    return nb::pending;
                }
            }

            return nb::Ready{nb::Empty{}};
        }
    };

    template <typename Serial>
    class DTExecutor {
        memory::Owned<Serial> serial_;

        bool command_sending_ = true;
        DTCommand<Serial> command_;
        FixedResponseWriter<2> response_;

      public:
        DTExecutor(
            Serial &&serial,
            ModemId dest,
            uint8_t length,
            nb::Promise<CommandWriter<Serial>> &&body
        )
            : serial_{memory::Owned{etl::move(serial)}},
              command_{dest, length, etl::move(body)} {}

        nb::Poll<bool> poll() {
            if (command_sending_) {
                POLL_UNWRAP_OR_RETURN(command_.poll(serial_));
                command_sending_ = false;
            }

            nb::stream::pipe(serial_.get(), response_);
            POLL_UNWRAP_OR_RETURN(response_.poll());
            return true;
        }
    };
} // namespace media::uhf
