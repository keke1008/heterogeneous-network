#pragma once

#include "../frame.h"
#include "./task.h"
#include <debug_assert.h>
#include <etl/circular_buffer.h>
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/stream/types.h>
#include <util/rand.h>
#include <util/tuple.h>
#include <util/visitor.h>

namespace net::link::uhf {
    class UHFExecutor {
        etl::optional<Task> task_;
        nb::stream::ReadableWritableStream &stream_;

        nb::Poll<void> wait_until_task_addable() const {
            bool task_addable = !task_.has_value()                // タスクを持っていない
                                && stream_.readable_count() == 0; // 何も受信していない
            return task_addable ? nb::ready() : nb::pending;
        }

      public:
        UHFExecutor() = delete;
        UHFExecutor(const UHFExecutor &) = delete;
        UHFExecutor(UHFExecutor &&) = default;
        UHFExecutor &operator=(const UHFExecutor &) = delete;
        UHFExecutor &operator=(UHFExecutor &&) = delete;

        UHFExecutor(nb::stream::ReadableWritableStream &stream) : stream_{stream} {}

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::UHF;
        }

        nb::Poll<void> include_route_information() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto task = IncludeRouteInformationTask{};
            task_.emplace(etl::move(task));
            return nb::ready();
        }

        inline nb::Poll<FrameTransmission> send_data(const Address &destination, uint8_t length) {
            DEBUG_ASSERT(destination.type() == AddressType::UHF);
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [frame, p_body, p_success] = FrameTransmission::make_frame_transmission();
            auto task = DataTransmissionTask{
                ModemId(destination), length, etl::move(p_body), etl::move(p_success)};
            task_.emplace(etl::move(task));
            return nb::ready(etl::move(frame));
        }

        inline nb::Poll<nb::Future<SerialNumber>> get_serial_number() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [f, p] = nb::make_future_promise_pair<SerialNumber>();
            auto task = GetSerialNumberTask{etl::move(p)};
            task_.emplace(etl::move(task));
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<void>> set_equipment_id(ModemId equipment_id) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [f, p] = nb::make_future_promise_pair<void>();
            auto task = SetEquipmentIdTask{equipment_id, etl::move(p)};
            task_.emplace(etl::move(task));
            return nb::ready(etl::move(f));
        }

        nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            if (!task_.has_value()) {
                if (stream_.readable_count() == 0) {
                    return nb::pending;
                }

                auto [frame, p_body, p_source] = FrameReception::make_frame_reception();
                auto task = DataReceivingTask{etl::move(p_body), etl::move(p_source)};
                task.poll(stream_);
                task_.emplace(etl::move(task));
                return nb::ready(etl::move(frame));
            }

            auto &task = task_.value();
            POLL_UNWRAP_OR_RETURN(etl::visit(
                util::Visitor{
                    [&](DataTransmissionTask &task) { return task.poll(stream_, time, rand); },
                    [&](auto &task) { return task.poll(stream_); }},
                task
            ));
            task_ = etl::nullopt;
            return nb::pending;
        }
    };
} // namespace net::link::uhf
