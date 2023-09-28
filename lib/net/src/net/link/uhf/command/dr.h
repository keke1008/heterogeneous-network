#pragma once

#include "../../link.h"
#include "../common.h"
#include <etl/functional.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/frame/service.h>
#include <serde/hex.h>

namespace net::link::uhf {
    class DRExecutor {
        enum class State : uint8_t {
            PrefixLength,
            Body,
            Route,
            Suffix,
        } state_{State::PrefixLength};

        nb::stream::FixedWritableBuffer<4> prefix_;
        nb::stream::FixedWritableBuffer<2> length_;
        etl::optional<net::frame::FrameReception<Address>> body_;
        nb::stream::FixedWritableBuffer<2> route_prefix_;
        nb::stream::FixedWritableBuffer<2> route_;
        nb::stream::FixedWritableBuffer<2> suffix_;

      public:
        DRExecutor() = default;

        nb::Poll<void> poll(
            net::frame::FrameService<Address> &service,
            nb::stream::ReadableWritableStream &stream
        ) {
            if (state_ == State::PrefixLength) {
                POLL_UNWRAP_OR_RETURN(nb::stream::write_all_from(stream, prefix_, length_));

                auto raw_length = POLL_UNWRAP_OR_RETURN(length_.poll());
                auto length = serde::hex::deserialize<uint8_t>(raw_length).value();
                body_ = POLL_MOVE_UNWRAP_OR_RETURN(service.notify_reception(length));
                state_ = State::Body;
            }

            if (state_ == State::Body) {
                POLL_UNWRAP_OR_RETURN(body_->writer.write_all_from(stream));
                state_ = State::Route;
            }

            if (state_ == State::Route) {
                POLL_UNWRAP_OR_RETURN(nb::stream::write_all_from(stream, route_prefix_, route_));
                auto raw_source = POLL_UNWRAP_OR_RETURN(route_.poll());
                auto source = serde::hex::deserialize<uint8_t>(raw_source).value();
                body_->source.set_value(Address(ModemId{source}));
                state_ = State::Suffix;
            }

            return suffix_.write_all_from(stream);
        }
    };
} // namespace net::link::uhf
