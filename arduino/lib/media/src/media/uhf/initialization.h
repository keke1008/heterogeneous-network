#pragma once

#include "./task.h"
#include <nb/poll.h>

namespace net::link::uhf {
    template <nb::AsyncReadableWritable RW>
    class Initializer {
        etl::optional<nb::Future<etl::optional<SerialNumber>>> serial_number_future_;
        etl::optional<ModemId> equipment_id_;

        enum class State : uint8_t {
            Initial,
            IncludeRouteInformation,
            GetSerialNumber,
            GotSerialNumber,
            SetEquipmentId,
        } state_{State::Initial};

      public:
        inline nb::Poll<etl::optional<ModemId>> execute(
            frame::FrameService &service,
            TaskExecutor<RW> &executor,
            util::Time &time,
            util::Rand &rand
        ) {
            if (state_ == State::Initial) {
                POLL_UNWRAP_OR_RETURN(executor.poll_task_addable());
                executor.template emplace<IncludeRouteInformationTask<RW>>(time);
                state_ = State::IncludeRouteInformation;
            }

            if (state_ == State::IncludeRouteInformation) {
                POLL_UNWRAP_OR_RETURN(executor.poll_task_addable());
                auto [f, p] = nb::make_future_promise_pair<etl::optional<SerialNumber>>();
                serial_number_future_ = etl::move(f);
                executor.template emplace<GetSerialNumberTask<RW>>(time, etl::move(p));
                state_ = State::GetSerialNumber;
            }

            if (state_ == State::GetSerialNumber) {
                POLL_UNWRAP_OR_RETURN(executor.poll_task_addable());
                if (serial_number_future_->never_receive_value()) {
                    return etl::optional<ModemId>{};
                }

                auto ref_opt_serial_number = POLL_UNWRAP_OR_RETURN(serial_number_future_->poll());
                if (!ref_opt_serial_number.get().has_value()) {
                    return etl::optional<ModemId>{};
                }
                auto span = etl::span(ref_opt_serial_number.get()->get());

                // シリアル番号の下2バイトを機器IDとする
                // 機器IDは16進数で表現される
                nb::AsyncSpanReadable span_reader{span.template last<2>()};
                AsyncModemIdDeserializer modem_id_deserializer;
                FASSERT(modem_id_deserializer.deserialize(span_reader).is_ready());

                equipment_id_ = modem_id_deserializer.result();
                executor.template emplace<SetEquipmentIdTask<RW>>(time, *equipment_id_);
                state_ = State::GotSerialNumber;
            }

            if (state_ == State::GotSerialNumber) {
                POLL_UNWRAP_OR_RETURN(executor.poll_task_addable());
                state_ = State::SetEquipmentId;
            }

            if (state_ == State::SetEquipmentId) {
                POLL_UNWRAP_OR_RETURN(executor.poll_task_addable());
                return equipment_id_;
            }

            return nb::pending;
        }
    };
} // namespace net::link::uhf
