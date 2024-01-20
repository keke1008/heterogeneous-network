#pragma once

#include "./task/discard_response.h"
#include "./task/get_serial_number.h"
#include "./task/include_route_information.h"
#include "./task/receive_data.h"
#include "./task/send_data.h"
#include "./task/set_equipment_id.h"

namespace media::uhf {
    static constexpr auto TASK_TIMEOUT = util::Duration::from_seconds(5);

    template <nb::AsyncReadableWritable RW>
    class Task {
        etl::optional<nb::Delay> timeout_;
        etl::variant<
            etl::monostate,
            SendDataTask<RW>,
            GetSerialNumberTask<RW>,
            SetEquipmentIdTask<RW>,
            IncludeRouteInformationTask<RW>,
            DiscardResponseTask<RW>>
            task_{};

      public:
        inline void clear() {
            task_.template emplace<etl::monostate>();
            timeout_.reset();
        }

        inline void clear_if_timeout(util::Time &time) {
            if (timeout_.has_value() && timeout_->poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("UHF task timeout: "), task_.index());
                clear();
            }
        }

        inline void interrupt() {
            TaskInterruptionResult result = etl::visit(
                util::Visitor{
                    [&](etl::monostate) { return TaskInterruptionResult::Interrupted; },
                    [&](auto &task) { return task.interrupt(); },
                },
                task_
            );
            if (result == TaskInterruptionResult::Aborted) {
                clear();
            } else {
                timeout_.reset();
            }
        }

        inline void resume(util::Time &time) {
            timeout_ = nb::Delay{time, TASK_TIMEOUT};
        }

        inline nb::Poll<void> poll_task_addable() const {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

        void execute(
            net::frame::FrameService &fs,
            nb::Lock<etl::reference_wrapper<RW>> &rw,
            net::link::FrameBroker &broker,
            util::Time &time,
            util::Rand &rand
        ) {
            nb::Poll<void> poll = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) -> nb::Poll<void> { return nb::pending; },
                    [&](ReceiveDataTask<RW> &task) { return task.execute(fs, broker, time); },
                    [&](SendDataTask<RW> &task) { return task.execute(rw, time, rand); },
                    [&](GetSerialNumberTask<RW> &task) { return task.execute(rw); },
                    [&](SetEquipmentIdTask<RW> &task) { return task.execute(rw); },
                    [&](IncludeRouteInformationTask<RW> &task) { return task.execute(rw); },
                    [&](DiscardResponseTask<RW> &task) { return task.execute(); },
                },
                task_
            );
            if (poll.is_ready()) {
                clear();
            }
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            static_assert(!etl::is_same_v<T, etl::monostate>);

            FASSERT(poll_task_addable().is_ready());
            task_.template emplace<T>(etl::forward<Args>(args)...);
            timeout_.emplace(time, TASK_TIMEOUT);
        }

        template <typename F>
        inline auto visit(F &&f) {
            return etl::visit(etl::forward<F>(f), task_);
        }
    };

    template <nb::AsyncReadableWritable RW>
    struct ReceiveTask {
        ReceiveDataTask<RW> receive_task_;
        nb::Delay timeout_;

        explicit ReceiveTask(nb::LockGuard<etl::reference_wrapper<RW>> &&rw, util::Time &time)
            : receive_task_{etl::move(rw)},
              timeout_{time, TASK_TIMEOUT} {}

        nb::Poll<void>
        execute(net::frame::FrameService &fs, net::link::FrameBroker &broker, util::Time &time) {
            if (timeout_.poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("UHF receive task timeout"));
                return nb::ready();
            }

            return receive_task_.execute(fs, broker, time);
        }
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        net::link::FrameBroker broker_;
        Task<RW> task_{};
        etl::optional<ReceiveTask<RW>> receive_task_{};

      public:
        TaskExecutor(const net::link::FrameBroker &broker) : broker_{broker} {}

        inline nb::Poll<void> poll_task_addable() {
            return task_.poll_task_addable();
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            return task_.template emplace<T, Args...>(time, etl::forward<Args>(args)...);
        }

        void execute(
            net::frame::FrameService &fs,
            nb::Lock<etl::reference_wrapper<RW>> &rw,
            util::Time &time,
            util::Rand &rand
        ) {
            if (receive_task_.has_value()) {
                auto poll = receive_task_->execute(fs, broker_, time);
                if (poll.is_pending()) {
                    return;
                }

                receive_task_.reset();
                task_.resume(time);
            }

            task_.clear_if_timeout(time);
            if (task_.poll_task_addable().is_ready()) {
                auto &&poll_frame = broker_.poll_get_send_requested_frame(net::link::AddressType::UHF);
                if (poll_frame.is_ready()) {
                    auto &&frame = UhfFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                    task_.template emplace<SendDataTask<RW>>(time, etl::move(frame), time, rand);
                }
            }

            task_.execute(fs, rw, broker_, time, rand);
        }

        void handle_response(util::Time &time, UhfResponse<RW> &&res) {
            if (res.type == UhfResponseType::DR) {
                FASSERT(!receive_task_.has_value());
                receive_task_.emplace(etl::move(res.body), time);
                task_.interrupt();
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
} // namespace media::uhf
