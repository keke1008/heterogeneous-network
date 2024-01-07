#pragma once

#include "../broker.h"
#include "./task/discard_response.h"
#include "./task/get_serial_number.h"
#include "./task/include_route_information.h"
#include "./task/receive_data.h"
#include "./task/send_data.h"
#include "./task/set_equipment_id.h"

namespace net::link::uhf {
    template <nb::AsyncReadableWritable RW>
    class Task {
        etl::optional<nb::Delay> timeout_;
        etl::variant<
            etl::monostate,
            ReceiveDataTask<RW>,
            SendDataTask<RW>,
            GetSerialNumberTask<RW>,
            SetEquipmentIdTask<RW>,
            IncludeRouteInformationTask<RW>,
            DiscardResponseTask<RW>>
            task_{};

        static constexpr auto TIMEOUT = util::Duration::from_millis(1000);

      public:
        inline void clear() {
            task_.template emplace<etl::monostate>();
            timeout_.reset();
        }

        inline void clear_if_timeout(util::Time &time) {
            if (timeout_.has_value() && timeout_->poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("UHF task timeout"));
                clear();
            }
        }

        inline nb::Poll<void> poll_task_addable() const {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            static_assert(!etl::is_same_v<T, etl::monostate>);

            FASSERT(poll_task_addable().is_ready());
            task_.template emplace<T>(etl::forward<Args>(args)...);
            timeout_.emplace(time, TIMEOUT);
        }

        template <typename F>
        inline auto visit(F &&f) {
            return etl::visit(etl::forward<F>(f), task_);
        }
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        FrameBroker broker_;
        Task<RW> task_{};

      public:
        TaskExecutor(const FrameBroker &broker) : broker_{broker} {}

        inline nb::Poll<void> poll_task_addable() {
            return task_.poll_task_addable();
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            return task_.template emplace<T, Args...>(time, etl::forward<Args>(args)...);
        }

        void execute(
            frame::FrameService &fs,
            nb::Lock<etl::reference_wrapper<RW>> &rw,
            util::Time &time,
            util::Rand &rand
        ) {
            task_.clear_if_timeout(time);

            if (task_.poll_task_addable().is_ready()) {
                auto &&poll_frame = broker_.poll_get_send_requested_frame(AddressType::UHF);
                if (poll_frame.is_ready()) {
                    auto &&frame = UhfFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                    task_.template emplace<SendDataTask<RW>>(time, etl::move(frame));
                }
            }

            nb::Poll<void> poll = task_.visit(util::Visitor{
                [&](etl::monostate &) -> nb::Poll<void> { return nb::pending; },
                [&](ReceiveDataTask<RW> &task) { return task.execute(fs, broker_, time); },
                [&](SendDataTask<RW> &task) { return task.execute(rw, time, rand); },
                [&](GetSerialNumberTask<RW> &task) { return task.execute(rw); },
                [&](SetEquipmentIdTask<RW> &task) { return task.execute(rw); },
                [&](IncludeRouteInformationTask<RW> &task) { return task.execute(rw); },
                [&](DiscardResponseTask<RW> &task) { return task.execute(); },
            });
            if (poll.is_ready()) {
                task_.clear();
            }
        }

        void handle_response(util::Time &time, UhfResponse<RW> &&res) {
            if (res.type == UhfResponseType::DR) {
                task_.clear();
                task_.template emplace<ReceiveDataTask<RW>>(time, etl::move(res.body));
                return;
            }

            UhfHandleResponseResult handle_result = task_.visit(util::Visitor{
                [&](etl::monostate) { return UhfHandleResponseResult::Handle; },
                [&](auto &task) { return task.handle_response(etl::move(res)); },
            });
            if (handle_result == UhfHandleResponseResult::Invalid) {
                task_.clear();
                task_.template emplace<DiscardResponseTask<RW>>(time, etl::move(res.body));
            }
        }
    };
} // namespace net::link::uhf
