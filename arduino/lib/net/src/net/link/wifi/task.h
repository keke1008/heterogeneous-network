#pragma once

#include "../broker.h"
#include "./control.h"
#include "./frame.h"
#include "./message.h"

namespace net::link::wifi {
    using Task = etl::variant<
        etl::monostate,
        Initialization,
        JoinAp,
        StartUdpServer,
        CloseUdpServer,
        SendData,
        GetIp,
        MessageHandler>;

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        RW &readable_writable_;
        FrameBroker broker_;
        Task task_;

      public:
        TaskExecutor(RW &rw, const FrameBroker &broker)
            : readable_writable_{rw},
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
            if (!readable_writable_.poll_readable(1).is_ready()) {
                task_.emplace<MessageHandler>();
                return;
            }

            auto &&poll_frame = broker_.poll_get_send_requested_frame(AddressType::IPv4);
            if (poll_frame.is_ready()) {
                auto &&wifi_frame = WifiDataFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                task_.emplace<SendData>(etl::move(wifi_frame));
            }
        }

        nb::Poll<etl::optional<WifiEvent>>
        on_hold_message_handler(frame::FrameService &frame_service) {
            auto &handler = etl::get<MessageHandler>(task_);
            auto &&event =
                POLL_MOVE_UNWRAP_OR_RETURN(handler.execute(frame_service, readable_writable_));
            return etl::visit(
                util::Visitor{
                    [&](etl::monostate &) -> etl::optional<WifiEvent> { return etl::nullopt; },
                    [&](WifiDataFrame &frame) -> etl::optional<WifiEvent> {
                        auto &&link_frame = LinkFrame(etl::move(frame));
                        broker_.poll_dispatch_received_frame(etl::move(link_frame));
                        return etl::nullopt;
                    },
                    [&](WifiEvent &event) -> etl::optional<WifiEvent> { return event; },
                },
                event
            );
        }

      public:
        etl::optional<WifiEvent> execute(frame::FrameService &frame_service) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                on_hold_monostate();
            }

            auto poll_opt_event = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) -> nb::Poll<etl::optional<WifiEvent>> {
                        return nb::pending;
                    },
                    [&](MessageHandler &task) -> nb::Poll<etl::optional<WifiEvent>> {
                        return on_hold_message_handler(frame_service);
                    },
                    [&](auto &task) -> nb::Poll<etl::optional<WifiEvent>> {
                        POLL_UNWRAP_OR_RETURN(task.execute(readable_writable_));
                        return etl::optional<WifiEvent>{etl::nullopt};
                    },
                },
                task_
            );

            if (poll_opt_event.is_ready()) {
                task_.emplace<etl::monostate>();
                return poll_opt_event.unwrap();
            } else {
                return etl::nullopt;
            }
        }
    };
} // namespace net::link::wifi
