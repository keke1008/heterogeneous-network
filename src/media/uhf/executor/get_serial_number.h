#pragma once

#include "../modem.h"
#include <etl/variant.h>
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/poll.h>
#include <nb/result.h>

namespace media::uhf::executor {
    enum class GetSerialNumberState : uint8_t {
        ResponseHeader,
        ResponseBody,
        Completed,
    };

    template <typename ReaderWriter>
    class GetSerialNumberExecutor {
        nb::Promise<modem::SerialNumber> promise_;
        nb::lock::Guard<ReaderWriter> reader_writer_;

        modem::GetSerialNumberCommand command_;

        GetSerialNumberState state_{GetSerialNumberState::ResponseHeader};
        modem::ResponseHeader<ReaderWriter> response_header_;
        modem::GetSerialNumberResponseBody response_body_;

      public:
        GetSerialNumberExecutor(
            nb::lock::Guard<ReaderWriter> &&reader_writer,
            nb::Promise<modem::SerialNumber> &&promise
        )
            : reader_writer_{etl::move(reader_writer)},
              promise_{etl::move(promise)} {}

        nb::Poll<nb::Result<nb::Empty, nb::Empty>> execute() {
            command_.write_to(*reader_writer_);

            switch (state_) {
            case GetSerialNumberState::ResponseHeader: {
                auto response_type =
                    POLL_UNWRAP_OR_RETURN(response_header_.read_from(*reader_writer_));
                if (!response_type.has_value() ||
                    response_type.value() != modem::ResponseType::GetSerialNumber) {
                    return nb::Err{nb::empty};
                }
                state_ = GetSerialNumberState::ResponseBody;
                [[fallthrough]];
            }
            case GetSerialNumberState::ResponseBody: {
                auto serial_number =
                    POLL_UNWRAP_OR_RETURN(response_body_.read_from(*reader_writer_));
                promise_.set_value(etl::move(serial_number));
                state_ = GetSerialNumberState::Completed;
                [[fallthrough]];
            }
            case GetSerialNumberState::Completed: {
                return nb::Ok{nb::empty};
            }
            }
        }
    };

    template <typename ReaderWriter>
    class GetSerialNumberTask {
        nb::Promise<modem::SerialNumber> promise_;

      public:
        GetSerialNumberTask(nb::Promise<modem::SerialNumber> &&promise)
            : promise_{etl::move(promise)} {}

        GetSerialNumberExecutor<ReaderWriter>
        into_executor(nb::lock::Guard<ReaderWriter> &&reader_writer) {
            return GetSerialNumberExecutor<ReaderWriter>{
                etl::move(reader_writer), etl::move(promise_)};
        }
    };
} // namespace media::uhf::executor
