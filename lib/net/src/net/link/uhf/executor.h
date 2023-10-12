#pragma once

#include "../address.h"
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
        etl::optional<ModemId> self_id_;
        etl::optional<Frame> received_frame_;

        nb::Poll<void> wait_until_task_addable() const {
            bool task_addable = !task_.has_value() // タスクを持っていない
                && stream_.readable_count() == 0;  // 何も受信していない
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
            self_id_ = equipment_id;

            auto [f, p] = nb::make_future_promise_pair<void>();
            auto task = SetEquipmentIdTask{equipment_id, etl::move(p)};
            task_.emplace(etl::move(task));
            return nb::ready(etl::move(f));
        }

        nb::Poll<void> send_frame(Frame &&frame) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());
            task_.emplace(DataTransmissionTask{etl::move(frame)});
            return nb::ready();
        }

        nb::Poll<Frame> receive_frame() {
            if (received_frame_.has_value()) {
                auto frame = etl::move(received_frame_.value());
                received_frame_ = etl::nullopt;
                return nb::ready(etl::move(frame));
            } else {
                return nb::pending;
            }
        }

        template <net::frame::IFrameService FrameService>
        void execute(FrameService &service, util::Time &time, util::Rand &rand) {
            if (!task_.has_value()) {
                if (stream_.readable_count() != 0) {
                    bool discard = received_frame_.has_value();
                    DataReceivingTask task{discard};
                    task.poll(service, stream_);
                    task_.emplace(etl::move(task));
                } else {
                    return;
                }
            }

            auto &task = task_.value();

            if (etl::holds_alternative<DataReceivingTask>(task)) {
                auto &receiving_task = etl::get<DataReceivingTask>(task);
                auto poll_opt_frame = receiving_task.poll(service, stream_);
                if (poll_opt_frame.is_ready()) {
                    task_ = etl::nullopt;
                    if (poll_opt_frame.value().has_value()) {
                        auto &frame = poll_opt_frame.value().value();
                        received_frame_ = etl::move(etl::move(frame));
                    }
                }
                return;
            }

            auto poll = etl::visit(
                util::Visitor{
                    [&](DataReceivingTask &task) { return nb::pending; },
                    [&](DataTransmissionTask &task) { return task.poll(stream_, time, rand); },
                    [&](auto &task) { return task.poll(stream_); }},
                task
            );
            if (poll.is_ready()) {
                task_ = etl::nullopt;
            }
        }
    };
} // namespace net::link::uhf
