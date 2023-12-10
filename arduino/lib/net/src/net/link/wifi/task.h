#pragma once

#include "../broker.h"
#include "./control.h"
#include "./frame.h"
#include "./message.h"
#include "./server.h"

namespace net::link::wifi {
    using Task = etl::variant<
        etl::monostate,
        Initialization,
        JoinAp,
        StartUdpServer,
        CloseUdpServer,
        SendWifiFrame,
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

      public:
        void execute(frame::FrameService &fs, LocalServerState &server) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                if (!readable_writable_.poll_readable(1).is_ready()) {
                    task_.emplace<MessageHandler>();
                } else {
                    auto &&poll_frame = broker_.poll_get_send_requested_frame(AddressType::IPv4);
                    if (poll_frame.is_pending()) {
                        return;
                    }
                    auto &&frame = WifiDataFrame::from_link_frame(etl::move(poll_frame.unwrap()));
                    task_.emplace<SendWifiFrame>(etl::move(frame));
                }
            }

            nb::Poll<etl::optional<WifiEvent>> &&poll_opt_event =
                etl::visit<nb::Poll<etl::optional<WifiEvent>>>(
                    util::Visitor{
                        [&](etl::monostate &) { return nb::pending; },
                        [&](MessageHandler &task) { return task.execute(fs, readable_writable_); },
                        [&](SendWifiFrame &task) { return task.execute(readable_writable_); },
                        [&](auto &task) -> nb::Poll<etl::optional<WifiEvent>> {
                            POLL_UNWRAP_OR_RETURN(task.execute(readable_writable_));
                            return etl::optional<WifiEvent>{etl::nullopt};
                        },
                    },
                    task_
                );

            if (poll_opt_event.is_pending()) {
                return;
            }
            task_.emplace<etl::monostate>();
            if (!poll_opt_event.unwrap().has_value()) {
                return;
            }

            etl::visit(
                util::Visitor{
                    [&](GotLocalIp &&) {
                        auto [f, p] = nb::make_future_promise_pair<WifiIpV4Address>();
                        server.set_local_address_future(etl::move(f));
                        task_.emplace<GetIp>(etl::move(p));
                    },
                    [&](SentDataFrame &&e) {
                        if (!server.has_global_address()) {
                            task_.emplace<SendWifiFrame>(
                                e.destination, WifiControlFrame{WifiGlobalAddressRequestFrame{}}
                            );
                        }
                    },
                    [&](DisconnectAp &&) { server.on_disconnect_ap(); },
                    [&](ReceiveDataFrame &&e) {
                        broker_.poll_dispatch_received_frame(LinkFrame(etl::move(e.frame)));
                    },
                    [&](ReceiveControlFrame &&e) {
                        etl::visit(
                            util::Visitor{
                                [&](WifiGlobalAddressRequestFrame &) {
                                    WifiGlobalAddressResponseFrame frame{.address = e.source};
                                    task_.emplace<SendWifiFrame>(e.source, WifiControlFrame{frame});
                                },
                                [&](WifiGlobalAddressResponseFrame &frame) {
                                    server.on_got_maybe_global_ip(frame.address);
                                }
                            },
                            e.frame
                        );
                    }
                },
                etl::move(poll_opt_event.unwrap().value())
            );
        }
    };
} // namespace net::link::wifi
