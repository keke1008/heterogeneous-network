#pragma once

#include "../frame.h"
#include "./task.h"
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <util/rand.h>
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

        template <net::frame::IFrameService<Address> FrameService>
        void execute(FrameService &service, util::Time &time, util::Rand &rand) {
            if (!task_.has_value()) {
                if (stream_.readable_count() != 0) {
                    DataReceivingTask task;
                    task.poll(service, stream_);
                    task_.emplace(etl::move(task));
                } else {
                    auto request = service.poll_transmission_request([](auto &request) {
                        return request.destination.type() == AddressType::UHF;
                    });
                    if (request.is_ready()) {
                        auto task = DataTransmissionTask{etl::move(request.unwrap())};
                        task_.emplace(etl::move(task));
                    }
                }
            }

            if (!task_.has_value()) {
                return;
            }

            auto &task = task_.value();
            auto poll = etl::visit(
                util::Visitor{
                    [&](DataTransmissionTask &task) { return task.poll(stream_, time, rand); },
                    [&](DataReceivingTask &task) { return task.poll(service, stream_); },
                    [&](auto &task) { return task.poll(stream_); }},
                task
            );
            if (poll.is_ready()) {
                task_ = etl::nullopt;
            }
        }
    };
} // namespace net::link::uhf
