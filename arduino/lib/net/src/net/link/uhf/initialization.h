#pragma once

#include "./task.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class Initializer {
        TaskExecutor &executor_;
        etl::optional<nb::Future<SerialNumber>> serial_number_future_;
        etl::optional<ModemId> equipment_id_;

        enum class State : uint8_t {
            Initial,
            IncludeRouteInformation,
            GetSerialNumber,
            GotSerialNumber,
            SetEquipmentId,
        } state_{State::Initial};

      public:
        explicit Initializer(TaskExecutor &executor) : executor_{executor} {}

        inline nb::Poll<ModemId>
        execute(frame::FrameService &service, util::Time &time, util::Rand &rand) {
            executor_.execute(service, time, rand);

            if (state_ == State::Initial) {
                POLL_UNWRAP_OR_RETURN(executor_.poll_task_addable());
                executor_.emplace<IncludeRouteInformationTask>();
                state_ = State::IncludeRouteInformation;
            }

            if (state_ == State::IncludeRouteInformation) {
                POLL_UNWRAP_OR_RETURN(executor_.poll_task_addable());
                auto [f, p] = nb::make_future_promise_pair<SerialNumber>();
                serial_number_future_ = etl::move(f);
                executor_.emplace<GetSerialNumberTask>(etl::move(p));
                state_ = State::GetSerialNumber;
            }

            if (state_ == State::GetSerialNumber) {
                POLL_UNWRAP_OR_RETURN(executor_.poll_task_addable());
                auto serial_number = POLL_UNWRAP_OR_RETURN(serial_number_future_->poll());
                auto span = etl::span(serial_number.get().get());

                // シリアル番号の下2バイトを機器IDとする
                equipment_id_ = ModemId{span.template last<2>()};
                executor_.emplace<SetEquipmentIdTask>(etl::move(*equipment_id_));
                state_ = State::GotSerialNumber;
            }

            if (state_ == State::GotSerialNumber) {
                POLL_UNWRAP_OR_RETURN(executor_.poll_task_addable());
                state_ = State::SetEquipmentId;
            }

            if (state_ != State::SetEquipmentId) {
                return nb::pending;
            }

            POLL_UNWRAP_OR_RETURN(executor_.poll_task_addable());
            return *equipment_id_;
        }
    };
} // namespace net::link::uhf
