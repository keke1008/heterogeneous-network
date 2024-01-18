#pragma once

#include "../broker.h"
#include "./constants.h"
#include "./control.h"
#include "./frame.h"
#include "./message.h"
#include "./server.h"

namespace net::link::wifi {
    class Task {
        using TaskVariant = etl::variant<
            etl::monostate,
            Initialization,
            JoinAp,
            StartUdpServer,
            CloseUdpServer,
            SendWifiFrame,
            GetIp,
            MessageHandler>;

        TaskVariant task_;
        etl::optional<nb::Delay> timeout_;

      public:
        template <typename T>
        inline bool is() const {
            return etl::holds_alternative<T>(task_);
        }

        inline bool is_empty() const {
            return etl::holds_alternative<etl::monostate>(task_);
        }

        template <typename T>
        inline T &get() {
            return etl::get<T>(task_);
        }

        inline void clear() {
            task_.emplace<etl::monostate>();
        }

        template <typename T, typename... Args>
        inline void emplace(util::Time &time, Args &&...args) {
            task_.emplace<T>(etl::forward<Args>(args)...);
            etl::visit(
                util::Visitor{
                    [&](etl::monostate &) { timeout_ = etl::nullopt; },
                    [&](JoinAp &) { timeout_ = nb::Delay(time, JOIN_AP_TIMEOUT); },
                    [&](auto &) { timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT); },
                },
                task_
            );
        }

        template <nb::AsyncReadableWritable RW>
        etl::optional<WifiEvent>
        execute(frame::FrameService &fs, util::Time &time, RW &readable_writable) {
            if (timeout_.has_value() && timeout_->poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("WIFI TASK TIMEOUT"));
                task_.emplace<etl::monostate>();
                timeout_ = etl::nullopt;
                return etl::optional<WifiEvent>{etl::nullopt};
            }

            nb::Poll<etl::optional<WifiEvent>> &&poll_opt_event =
                etl::visit<nb::Poll<etl::optional<WifiEvent>>>(
                    util::Visitor{
                        [&](etl::monostate &) { return nb::pending; },
                        [&](MessageHandler &task) { return task.execute(fs, readable_writable); },
                        [&](SendWifiFrame &task) { return task.execute(readable_writable); },
                        [&](auto &task) -> nb::Poll<etl::optional<WifiEvent>> {
                            POLL_UNWRAP_OR_RETURN(task.execute(readable_writable));
                            return etl::optional<WifiEvent>{etl::nullopt};
                        },
                    },
                    task_
                );

            if (poll_opt_event.is_pending()) {
                return etl::nullopt;
            } else {
                task_.emplace<etl::monostate>();
                return etl::move(poll_opt_event.unwrap());
            }
        }
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        RW &readable_writable_;
        FrameBroker broker_;
        Task task_{};

      public:
        TaskExecutor(RW &rw, const FrameBroker &broker) : readable_writable_{rw}, broker_{broker} {}

        inline nb::Poll<void> poll_task_addable() {
            return task_.is_empty() ? nb::ready() : nb::pending;
        }

      public:
        template <typename T, typename... Args>
        inline void emplace_task(util::Time &time, Args &&...args) {
            task_.emplace<T>(time, etl::forward<Args>(args)...);
        }

        void execute(frame::FrameService &fs, LocalServerState &server, util::Time &time) {
            if (task_.is_empty()) {
                if (readable_writable_.poll_readable(1).is_ready()) {
                    task_.emplace<MessageHandler>(time);
                } else {
                    auto &&poll_frame = broker_.poll_get_send_requested_frame(AddressType::IPv4);
                    if (poll_frame.is_pending()) {
                        return;
                    }
                    auto &&frame = WifiDataFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                    task_.emplace<SendWifiFrame>(time, etl::move(frame));
                }
            }

            etl::optional<WifiEvent> &&opt_event = task_.execute(fs, time, readable_writable_);
            if (!opt_event.has_value()) {
                return;
            }

            etl::visit(
                util::Visitor{
                    [&](GotLocalIp &&) {
                        auto [f, p] = nb::make_future_promise_pair<WifiIpV4Address>();
                        server.set_local_address_future(etl::move(f));
                        task_.emplace<GetIp>(time, etl::move(p));
                    },
                    [&](SentDataFrame &&e) {}, [&](DisconnectAp &&) { server.on_disconnect_ap(); },
                    [&](ReceiveDataFrame &&e) {
                        broker_.poll_dispatch_received_frame(LinkFrame(etl::move(e.frame)), time);
                    },
                    [&](ReceiveControlFrame &&e) {
                        etl::visit(
                            util::Visitor{
                                [&](WifiGlobalAddressRequestFrame &) {},
                                [&](WifiGlobalAddressResponseFrame &frame) {
                                    server.on_got_maybe_global_ip(frame.address);
                                }
                            },
                            e.frame
                        );
                    }
                },
                etl::move(opt_event.value())
            );
        }
    };
} // namespace net::link::wifi
