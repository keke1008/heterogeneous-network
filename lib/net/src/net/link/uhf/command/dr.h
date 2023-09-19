#pragma once

#include <etl/functional.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/hex.h>

namespace net::link::uhf {
    class ResponseReader final : public nb::stream::ReadableStream {
        etl::reference_wrapper<nb::stream::ReadableStream> stream_;
        nb::Promise<void> barrier_;
        uint8_t total_length_;

        inline nb::stream::ReadableStream &stream() {
            return stream_.get();
        }

        inline const nb::stream::ReadableStream &stream() const {
            return stream_.get();
        }

      public:
        ResponseReader(
            nb::stream::ReadableWritableStream &stream,
            nb::Promise<void> &&barrier,
            uint8_t total_length
        )
            : stream_{etl::ref(stream)},
              barrier_{etl::move(barrier)},
              total_length_{total_length} {}

        inline uint8_t readable_count() const {
            return stream().readable_count();
        }

        inline uint8_t read() {
            return stream().read();
        }

        inline void read(etl::span<uint8_t> buffer) {
            stream().read(buffer);
        }

        inline void close() {
            barrier_.set_value();
        }

        inline constexpr uint8_t total_length() const {
            return total_length_;
        }
    };

    class DRExecutor {
        enum class State : uint8_t {
            PrefixLength,
            Body,
            Suffix,
        } state_{State::PrefixLength};

        nb::stream::FixedWritableBuffer<4> prefix_;
        nb::stream::FixedWritableBuffer<2> length_;
        nb::Promise<ResponseReader> body_;
        etl::optional<nb::Future<void>> barrier_;
        nb::stream::FixedWritableBuffer<2> suffix_;

      public:
        DRExecutor(nb::Promise<ResponseReader> &&body) : body_{etl::move(body)} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            if (state_ == State::PrefixLength) {
                POLL_UNWRAP_OR_RETURN(nb::stream::write_all_from(stream, prefix_, length_));

                auto raw_length = POLL_UNWRAP_OR_RETURN(length_.poll());
                auto length = serde::hex::deserialize<uint8_t>(raw_length).value();
                auto [barrier, barrier_promise] = nb::make_future_promise_pair<void>();
                ResponseReader reader{etl::ref(stream), etl::move(barrier_promise), length};
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
