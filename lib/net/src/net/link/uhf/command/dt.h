#pragma once

#include "../common.h"
#include "./common.h"
#include "serde/hex.h"
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <nb/time.h>
#include <util/time.h>

namespace net::link::uhf {
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
        enum class State : uint8_t {
            CommandSending,
            CommandResponse,
            WaitingForInformationResponse,
            InformationResponse,
        } state_{State::CommandSending};

        DTCommand<Serial> command_;
        FixedResponseWriter<2> command_response_;
        FixedResponseWriter<2> information_response_;

        etl::optional<nb::Delay> information_response_timeout_;

      public:
        DTExecutor(ModemId dest, uint8_t length, nb::Promise<CommandWriter<Serial>> &&body)
            : command_{dest, length, etl::move(body)} {}

        template <typename Time>
        nb::Poll<bool> poll(memory::Owned<Serial> &serial, Time &time) {
            if (state_ == State::CommandSending) {
                POLL_UNWRAP_OR_RETURN(command_.poll(serial));

                state_ = State::CommandResponse;
            }

            if (state_ == State::CommandResponse) {
                nb::stream::pipe(serial.get(), command_response_);
                POLL_UNWRAP_OR_RETURN(command_response_.poll());

                state_ = State::WaitingForInformationResponse;

                // 説明書には6msとあるが、余裕をもって10msにしておく
                auto timeout = util::Duration::from_millis(10);
                information_response_timeout_ = nb::Delay{time, timeout};
            }

            if (state_ == State::WaitingForInformationResponse) {
                nb::stream::pipe(serial.get(), information_response_);
                if (!information_response_.has_written()) {
                    POLL_UNWRAP_OR_RETURN(information_response_timeout_->poll(time));
                    return nb::Ready{true};
                }

                state_ = State::InformationResponse;
            }

            if (state_ == State::InformationResponse) {
                nb::stream::pipe(serial.get(), information_response_);
                POLL_UNWRAP_OR_RETURN(information_response_.poll());
            }

            return nb::Ready{false};
        }
    };
} // namespace net::link::uhf
