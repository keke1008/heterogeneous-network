#pragma once

#include "../common.h"
#include "../modem.h"

namespace media::uhf::executor {
    enum class SetEquipmentIdState {
        ReadResponseHeader,
        ReadResponseBody,
        Complete,
    };

    template <typename ReaderWriter>
    class SetEquipmentIdExecutor {
        nb::Promise<nb::Empty> promise_;
        nb::lock::Guard<ReaderWriter> reader_writer_;

        modem::SetEquipmentIdCommand command_;

        SetEquipmentIdState state_{SetEquipmentIdState::ReadResponseHeader};
        modem::ResponseHeader<ReaderWriter> response_header_;
        modem::SetEquipmentIdResponseBody response_body_;

      public:
        SetEquipmentIdExecutor(
            nb::lock::Guard<ReaderWriter> &&reader_writer,
            nb::Promise<nb::Empty> &&promise,
            common::ModemId equipment_id
        )
            : reader_writer_{etl::move(reader_writer)},
              promise_{etl::move(promise)},
              command_{equipment_id} {}

        nb::Poll<nb::Result<nb::Empty, nb::Empty>> execute() {
            command_.write_to(*reader_writer_);

            switch (state_) {
            case SetEquipmentIdState::ReadResponseHeader: {
                auto response_type =
                    POLL_UNWRAP_OR_RETURN(response_header_.read_from(*reader_writer_));
                if (!response_type.has_value() ||
                    response_type.value() != modem::ResponseType::SetEquipmentId) {
                    return nb::Err{nb::empty};
                }
                state_ = SetEquipmentIdState::ReadResponseBody;
                [[fallthrough]];
            }
            case SetEquipmentIdState::ReadResponseBody: {
                POLL_UNWRAP_OR_RETURN(response_body_.read_from(*reader_writer_));
                promise_.set_value(nb::empty);
                state_ = SetEquipmentIdState::Complete;
                [[fallthrough]];
            }
            case SetEquipmentIdState::Complete: {
                return nb::Ok{nb::empty};
            }
            }
        }
    };

    template <typename ReaderWriter>
    class SetEquipmentIdTask {
        nb::Promise<nb::Empty> promise_;
        common::ModemId equipment_id_;

      public:
        SetEquipmentIdTask(nb::Promise<nb::Empty> &&promise, common::ModemId equipment_id)
            : promise_{etl::move(promise)},
              equipment_id_{equipment_id} {}

        SetEquipmentIdExecutor<ReaderWriter>
        make_executor(nb::lock::Guard<ReaderWriter> &&reader_writer) {
            return SetEquipmentIdExecutor<ReaderWriter>{
                etl::move(reader_writer), etl::move(promise_), equipment_id_};
        }
    };
} // namespace media::uhf::executor
