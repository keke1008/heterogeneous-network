#pragma once

#include "../broker.h"
#include "./task/data_receiving.h"
#include "./task/data_transmission.h"
#include "./task/get_serial_number.h"
#include "./task/include_route_information.h"
#include "./task/set_equipment_id.h"
#include <etl/variant.h>

namespace net::link::uhf {
    using Task = etl::variant<
        etl::monostate,
        DataReceivingTask,
        DataTransmissionTask,
        GetSerialNumberTask,
        SetEquipmentIdTask,
        IncludeRouteInformationTask>;

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        RW &readable_writable_;
        FrameBroker broker_;
        Task task_;

      public:
        TaskExecutor(RW &readable_writable, const FrameBroker &broker)
            : readable_writable_{readable_writable},
              broker_{broker},
              task_{etl::monostate()} {}

        inline nb::Poll<void> poll_task_addable() {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

        template <typename T, typename... Args>
        inline void emplace(Args &&...args) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_.emplace<T>(etl::forward<Args>(args)...);
            }
        }

      private:
        void on_hold_monostate() {
            if (readable_writable_.poll_readable(1).is_ready()) {
                this->task_.template emplace<DataReceivingTask>();
                return;
            }

            auto poll_frame = this->broker_.poll_get_send_requested_frame(AddressType::UHF);
            if (poll_frame.is_ready()) {
                this->task_.template emplace<DataTransmissionTask>(
                    UhfFrame::from_link_frame(etl::move(poll_frame.unwrap()))
                );
            }
        }

        nb::Poll<void>
        poll_execute_task(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            return etl::visit(
                util::Visitor{
                    [&](etl::monostate &) -> nb::Poll<void> { return nb::pending; },
                    [&](DataReceivingTask &task) -> nb::Poll<void> {
                        auto &&frame =
                            POLL_MOVE_UNWRAP_OR_RETURN(task.poll(frame_service, readable_writable_)
                            );

                        // brokerの保持する受信フレームが満杯である場合は、受信フレームを破棄する
                        this->broker_.poll_dispatch_received_frame(LinkFrame(etl::move(frame)));
                        this->task_.template emplace<etl::monostate>();
                        return nb::ready();
                    },
                    [&](DataTransmissionTask &task) {
                        return task.poll(readable_writable_, time, rand);
                    },
                    [&](auto &task) { return task.poll(readable_writable_); },
                },
                task_
            );
        }

      public:
        void execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                on_hold_monostate();
            }

            if (poll_execute_task(frame_service, time, rand).is_ready()) {
                this->task_.template emplace<etl::monostate>();
            }
        }
    };
} // namespace net::link::uhf
