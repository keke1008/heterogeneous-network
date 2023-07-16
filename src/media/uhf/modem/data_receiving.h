#pragma once

#include "../common.h"
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/tuple.h>

namespace media::uhf::modem {
    enum class DataReceivingState : uint8_t {
        ReceiveLength,
        ReceiveData,
        DiscardRemainingData,
        ReceiveTerminator,
        Completed,
    };

    template <typename Reader>
    class DataReceivingResponseBody {
        DataReceivingState state_{DataReceivingState::ReceiveLength};

        nb::stream::TinyByteWriter<2> length_;
        nb::Promise<common::DataReader<Reader>> promise_;
        etl::optional<nb::Future<uint8_t>> remain_bytes_{etl::nullopt};
        nb::stream::DiscardingStreamWriter discarding_data_{0};
        nb::stream::DiscardingStreamWriter terminator_{2};

      public:
        DataReceivingResponseBody(nb::Promise<common::DataReader<Reader>> &&promise)
            : promise_{etl::move(promise)} {}

        nb::Poll<nb::Empty> read_from(memory::Owned<nb::lock::Guard<Reader>> &serial) {
            switch (state_) {
            case DataReceivingState::ReceiveLength: {
                nb::stream::pipe(*serial.get(), length_);
                auto length_bytes = POLL_UNWRAP_OR_RETURN(length_.poll());
                uint8_t length = serde::hex::deserialize<uint8_t>(length_bytes).value();

                state_ = DataReceivingState::ReceiveData;
                auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
                remain_bytes_ = etl::move(future);
                promise_.set_value(common::DataReader<Reader>{
                    etl::move(serial.try_create_pair().value()), length, etl::move(promise)});
                [[fallthrough]];
            }
            case DataReceivingState::ReceiveData: {
                auto remain_bytes = POLL_UNWRAP_OR_RETURN(remain_bytes_->get());
                discarding_data_ = nb::stream::DiscardingStreamWriter{remain_bytes};
                state_ = DataReceivingState::DiscardRemainingData;
                [[fallthrough]];
            }
            case DataReceivingState::DiscardRemainingData: {
                nb::stream::pipe(*serial.get(), discarding_data_);
                if (!discarding_data_.is_writer_closed()) {
                    return nb::pending;
                }
                state_ = DataReceivingState::ReceiveTerminator;
                [[fallthrough]];
            }
            case DataReceivingState::ReceiveTerminator: {
                nb::stream::pipe(*serial.get(), terminator_);
                if (!terminator_.is_writer_closed()) {
                    return nb::pending;
                }
                state_ = DataReceivingState::Completed;
                [[fallthrough]];
            }
            case DataReceivingState::Completed: {
                return nb::Ready{nb::empty};
            }
            }
            return nb::Ready{nb::empty};
        };
    };
} // namespace media::uhf::modem
