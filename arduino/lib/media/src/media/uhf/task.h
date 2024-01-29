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
    class InterruptibleTask {
        etl::optional<nb::Delay> timeout_;
        etl::variant<
            etl::monostate,
            SendDataTask<RW>,
            GetSerialNumberTask<RW>,
            SetEquipmentIdTask<RW>,
            IncludeRouteInformationTask<RW>>
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
            if (!timeout_.has_value() && !etl::holds_alternative<etl::monostate>(task_)) {
                timeout_ = nb::Delay{time, TASK_TIMEOUT};
            }
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

        nb::Poll<etl::expected<void, ReceiveDataAborted<RW>>>
        execute(net::frame::FrameService &fs, net::link::FrameBroker &broker, util::Time &time) {
            if (timeout_.poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("UHF receive task timeout"));
                return etl::expected<void, ReceiveDataAborted<RW>>{
                    etl::unexpected<ReceiveDataAborted<RW>>{
                        ReceiveDataAborted<RW>{etl::move(receive_task_).take_rw()}
                    }
                };
            }

            return receive_task_.execute(fs, broker, time);
        }
    };

    template <nb::AsyncReadableWritable RW>
    class ExclusiveTask {
        etl::variant<ReceiveTask<RW>, DiscardResponseTask<RW>> task_;

        explicit ExclusiveTask(etl::variant<ReceiveTask<RW>, DiscardResponseTask<RW>> &&task)
            : task_{etl::move(task)} {}

      public:
        static inline ExclusiveTask<RW>
        receive(nb::LockGuard<etl::reference_wrapper<RW>> &&rw, util::Time &time) {
            return ExclusiveTask<RW>{ReceiveTask<RW>{etl::move(rw), time}};
        }

        static inline ExclusiveTask<RW>
        discard_response(nb::LockGuard<etl::reference_wrapper<RW>> &&rw) {
            return ExclusiveTask<RW>{DiscardResponseTask<RW>{etl::move(rw)}};
        }

        nb::Poll<void>
        execute(net::frame::FrameService &fs, net::link::FrameBroker &broker, util::Time &time) {
            if (etl::holds_alternative<ReceiveTask<RW>>(task_)) {
                ReceiveTask<RW> &task = etl::get<ReceiveTask<RW>>(task_);
                auto result = POLL_UNWRAP_OR_RETURN(task.execute(fs, broker, time));
                if (result.has_value()) {
                    return nb::ready();
                } else {
                    auto &&rw = result.error().rw;
                    task_.template emplace<DiscardResponseTask<RW>>(etl::move(rw));
                }
            }

            auto &task = etl::get<DiscardResponseTask<RW>>(task_);
            return task.execute();
        }
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        memory::Static<net::link::FrameBroker> &broker_;
        InterruptibleTask<RW> interruptible_task_{};
        etl::optional<ExclusiveTask<RW>> exclusive_task_{};

      public:
        TaskExecutor(memory::Static<net::link::FrameBroker> &broker) : broker_{broker} {}

        inline nb::Poll<void> poll_task_addable() {
            return interruptible_task_.poll_task_addable();
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            return interruptible_task_.template emplace<T, Args...>(
                time, etl::forward<Args>(args)...
            );
        }

        void execute(
            net::frame::FrameService &fs,
            nb::Lock<etl::reference_wrapper<RW>> &rw,
            util::Time &time,
            util::Rand &rand
        ) {
            if (exclusive_task_.has_value()) {
                auto poll = exclusive_task_->execute(fs, *broker_, time);
                if (poll.is_pending()) {
                    return;
                }

                exclusive_task_.reset();
                interruptible_task_.resume(time);
            }

            interruptible_task_.clear_if_timeout(time);
            if (interruptible_task_.poll_task_addable().is_ready()) {
                auto &&poll_frame =
                    broker_->poll_get_send_requested_frame(net::link::AddressType::UHF);
                if (poll_frame.is_ready()) {
                    auto &&frame = UhfFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                    interruptible_task_.template emplace<SendDataTask<RW>>(
                        time, etl::move(frame), time, rand
                    );
                }
            }

            interruptible_task_.execute(fs, rw, *broker_, time, rand);
        }

        void handle_response(util::Time &time, UhfResponse<RW> &&res) {
            FASSERT(!exclusive_task_.has_value());

            if (res.type == UhfResponseType::DR) {
                exclusive_task_ = ExclusiveTask<RW>::receive(etl::move(res.body), time);
                interruptible_task_.interrupt();
                return;
            }

            UhfHandleResponseResult handle_result = interruptible_task_.visit(util::Visitor{
                [&](etl::monostate) { return UhfHandleResponseResult::Handle; },
                [&](auto &task) { return task.handle_response(etl::move(res)); },
            });
            if (handle_result == UhfHandleResponseResult::Invalid) {
                interruptible_task_.clear();
                exclusive_task_ = ExclusiveTask<RW>::discard_response(etl::move(res.body));
            }
        }
    };
} // namespace media::uhf
