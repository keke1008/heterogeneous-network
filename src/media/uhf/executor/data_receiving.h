#pragma once

#include "../common.h"
#include "../modem.h"
#include "media/uhf/modem/data_receiving.h"
#include <etl/optional.h>
#include <memory/pair_shared.h>
#include <nb/lock.h>
#include <nb/poll.h>

namespace media::uhf::executor {
    enum class DataReceivingState : uint8_t {
        Initial,
        HeaderReceiving,
        DataReceiving,
        Completed,
    };

    template <typename Reader>
    class DataReceivingExecutor {
        memory::Owned<nb::lock::Guard<Reader>> reader_;

        DataReceivingState state_{DataReceivingState::Initial};
        modem::ResponseHeader<Reader> response_header_;
        modem::DataReceivingResponseBody<Reader> response_body_;

      public:
        DataReceivingExecutor(
            nb::lock::Guard<Reader> &&reader,
            nb::Promise<common::DataReader<Reader>> &&promise
        )
            : reader_{memory::Owned{etl::move(reader)}},
              response_body_{etl::move(promise)} {}

        nb::Poll<nb::Result<nb::Empty, nb::Empty>> execute() {
            switch (state_) {
            case DataReceivingState::Initial: {
                if (!reader_.get()->is_readable()) {
                    return nb::Err{nb::empty};
                }
                state_ = DataReceivingState::HeaderReceiving;
                [[fallthrough]];
            }
            case DataReceivingState::HeaderReceiving: {
                auto response_type =
                    POLL_UNWRAP_OR_RETURN(response_header_.read_from(*reader_.get()));
                if (!response_type.has_value() ||
                    response_type.value() != modem::ResponseType::DataReceiving) {
                    return nb::Err{nb::empty};
                }
                state_ = DataReceivingState::DataReceiving;
                [[fallthrough]];
            }
            case DataReceivingState::DataReceiving: {
                POLL_UNWRAP_OR_RETURN(response_body_.read_from(reader_));
                state_ = DataReceivingState::Completed;
                [[fallthrough]];
            }
            case DataReceivingState::Completed: {
                return nb::Ok{nb::empty};
            }
            }
        }
    };

    template <typename Reader>
    class DataReceivingTask {
        nb::Promise<common::DataReader<Reader>> promise_;

      public:
        DataReceivingTask(nb::Promise<common::DataReader<Reader>> promise)
            : promise_{etl::move(promise)} {}

        DataReceivingExecutor<Reader> into_executor(memory::Owned<nb::lock::Guard<Reader>> reader) {
            return DataReceivingExecutor<Reader>{etl::move(reader), etl::move(promise_)};
        }
    };
} // namespace media::uhf::executor
