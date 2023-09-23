#pragma once

#include "../../../link/frame.h"
#include <etl/functional.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/hex.h>

namespace net::link::uhf {
    class DRExecutor {
        enum class State : uint8_t {
            PrefixLength,
            Body,
            Suffix,
        } state_{State::PrefixLength};

        nb::stream::FixedWritableBuffer<4> prefix_;
        nb::stream::FixedWritableBuffer<2> length_;
        nb::Promise<DataReader> body_;
        etl::optional<nb::Future<void>> barrier_;
        nb::stream::FixedWritableBuffer<2> suffix_;

      public:
        DRExecutor(nb::Promise<DataReader> &&body) : body_{etl::move(body)} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            if (state_ == State::PrefixLength) {
                POLL_UNWRAP_OR_RETURN(nb::stream::write_all_from(stream, prefix_, length_));

                auto raw_length = POLL_UNWRAP_OR_RETURN(length_.poll());
                auto length = serde::hex::deserialize<uint8_t>(raw_length).value();
                auto [barrier, barrier_promise] = nb::make_future_promise_pair<void>();
                DataReader reader{etl::ref(stream), etl::move(barrier_promise), length};
                body_.set_value(etl::move(reader));
                barrier_ = etl::move(barrier);
                state_ = State::Body;
            }

            if (state_ == State::Body) {
                POLL_UNWRAP_OR_RETURN(barrier_.value().poll());
                state_ = State::Suffix;
            }

            return suffix_.write_all_from(stream);
        }
    };
} // namespace net::link::uhf
