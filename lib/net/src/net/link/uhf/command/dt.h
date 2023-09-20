#pragma once

#include "../common.h"
#include "./common.h"
#include <nb/barrier.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <nb/time.h>
#include <serde/hex.h>
#include <util/time.h>

namespace net::link::uhf {
    class CommandWriter final : public nb::stream::WritableStream {
        etl::reference_wrapper<nb::stream::WritableStream> stream_;
        nb::Promise<void> barrier_;

        inline nb::stream::WritableStream &serial() {
            return stream_.get();
        }

        inline const nb::stream::WritableStream &serial() const {
            return stream_.get();
        }

      public:
        CommandWriter(
            etl::reference_wrapper<nb::stream::ReadableWritableStream> stream,
            nb::Promise<void> &&barrier

        )
            : stream_{stream},
              barrier_{etl::move(barrier)} {}

        inline uint8_t writable_count() const {
            return serial().writable_count();
        }

        inline bool write(uint8_t byte) {
            return serial().write(byte);
        }

        inline bool write(etl::span<uint8_t> buffer) {
            return serial().write(buffer);
        }

        inline void close() {
            barrier_.set_value();
        }
    };

    class DTCommand {
        enum class State : uint8_t {
            PrefixLength,
            Body,
            RouteSuffix,
        } state_{State::PrefixLength};

        nb::stream::FixedReadableBuffer<5> prefix_length_;
        etl::optional<nb::Future<void>> barrier_;
        nb::Promise<CommandWriter> body_;
        nb::stream::FixedReadableBuffer<6> route_suffix_;

      public:
        explicit DTCommand(ModemId dest, uint8_t length, nb::Promise<CommandWriter> body)
            : prefix_length_{'@', 'D', 'T', serde::hex::serialize(length)},
              body_{etl::move(body)},
              route_suffix_{'/', 'R', dest.span(), '\r', '\n'} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            if (state_ == State::PrefixLength) {
                POLL_UNWRAP_OR_RETURN(prefix_length_.read_all_into(stream));

                auto [barrier, barrier_promise] = nb::make_future_promise_pair<void>();
                CommandWriter writer{etl::ref(stream), etl::move(barrier_promise)};
                body_.set_value(etl::move(writer));
                barrier_ = etl::move(barrier);
                state_ = State::Body;
            }

            if (state_ == State::Body) {
                POLL_UNWRAP_OR_RETURN(barrier_.value().poll());
                state_ = State::RouteSuffix;
            }

            return route_suffix_.read_all_into(stream);
        }
    };

    class DTExecutor {
        enum class State : uint8_t {
            CommandSending,
            CommandResponse,
            WaitingForInformationResponse,
            InformationResponse,
        } state_{State::CommandSending};

        DTCommand command_;
        FixedResponseWriter<2> command_response_;
        FixedResponseWriter<2> information_response_;

        etl::optional<nb::Delay> information_response_timeout_;

      public:
        DTExecutor(ModemId dest, uint8_t length, nb::Promise<CommandWriter> &&body)
            : command_{dest, length, etl::move(body)} {}

        nb::Poll<bool> poll(nb::stream::ReadableWritableStream &stream, util::Time &time) {
            if (state_ == State::CommandSending) {
                POLL_UNWRAP_OR_RETURN(command_.poll(stream));
                state_ = State::CommandResponse;
            }

            if (state_ == State::CommandResponse) {
                POLL_UNWRAP_OR_RETURN(command_response_.write_all_from(stream));

                // 説明書には6msとあるが、余裕をもって10msにしておく
                auto timeout = util::Duration::from_millis(10);
                information_response_timeout_ = nb::Delay{time, timeout};
                state_ = State::WaitingForInformationResponse;
            }

            if (state_ == State::WaitingForInformationResponse) {
                information_response_.write_all_from(stream);
                if (!information_response_.has_written()) {
                    POLL_UNWRAP_OR_RETURN(information_response_timeout_->poll(time));
                    return nb::ready(true);
                }

                state_ = State::InformationResponse;
            }

            POLL_UNWRAP_OR_RETURN(information_response_.write_all_from(stream));
            return nb::ready(false);
        }
    };
} // namespace net::link::uhf
