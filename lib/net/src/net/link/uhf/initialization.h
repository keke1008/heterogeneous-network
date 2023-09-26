#pragma once

#include "./executor.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class Initializer {
        UHFExecutor &executor_;

        etl::optional<nb::Future<SerialNumber>> serial_number_future_;
        etl::optional<ModemId> equipment_id_;
        etl::optional<nb::Future<void>> set_equipment_id_completed_;

        enum class State : uint8_t {
            Initial,
            IncludeRouteInformation,
            GetSerialNumber,
            GotSerialNumber,
            SetEquipmentId,
        } state_{State::Initial};

      public:
        explicit Initializer(UHFExecutor &executor) : executor_{executor} {}

        inline nb::Poll<void> execute(util::Time &time, util::Rand &rand) {
            executor_.execute(time, rand);

            if (state_ == State::Initial) {
                executor_.include_route_information();
                state_ = State::IncludeRouteInformation;
            }

            if (state_ == State::IncludeRouteInformation) {
                serial_number_future_ = POLL_MOVE_UNWRAP_OR_RETURN(executor_.get_serial_number());
            }

            if (state_ == State::GetSerialNumber) {
                auto serial_number = POLL_UNWRAP_OR_RETURN(serial_number_future_->poll());
                auto span = etl::span(serial_number.get().get());

                equipment_id_ = ModemId{span.last<2>()}; // シリアル番号の下2バイトを機器IDとする
                state_ = State::GotSerialNumber;
            }

            if (state_ == State::GotSerialNumber) {
                set_equipment_id_completed_ =
                    POLL_MOVE_UNWRAP_OR_RETURN(executor_.set_equipment_id(equipment_id_.value()));
                state_ = State::SetEquipmentId;
            }

            return set_equipment_id_completed_.value().poll();
        }
    };
} // namespace net::link::uhf
