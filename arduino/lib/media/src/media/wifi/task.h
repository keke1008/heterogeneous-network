#pragma once

#include "./constants.h"
#include "./control.h"
#include "./frame.h"
#include "./message.h"
#include "./server.h"
#include <net/link.h>

namespace media::wifi {
    template <nb::AsyncReadable R, nb::AsyncWritable W>
    class Task {
        using TaskVariant = etl::variant<
            etl::monostate,
            Initialization<R, W>,
            JoinApControl<R, W>,
            StartUdpServerControl<R, W>,
            CloseUdpServerControl<R, W>,
            SendWifiFrameControl<R, W>,
            GetIpControl<R, W>>;

        TaskVariant task_{};
        etl::optional<nb::Delay> timeout_;
        memory::Static<W> &writable_;

      public:
        explicit Task(memory::Static<W> &writable) : writable_{writable} {}

        inline nb::Poll<void> poll_task_addable() const {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

        inline nb::Poll<nb::Future<bool>> poll_emplace_initialization(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_.template emplace<Initialization<R, W>>(writable_, etl::move(p));
            timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT);
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> poll_emplace_join_ap(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password,
            util::Time &time
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_.template emplace<JoinApControl<R, W>>(etl::move(p), writable_, ssid, password);
            timeout_ = nb::Delay(time, JOIN_AP_TIMEOUT);
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>>
        poll_emplace_start_udp_server(UdpPort port, util::Time &time) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_.template emplace<StartUdpServerControl<R, W>>(etl::move(p), writable_, port);
            timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT);
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> poll_emplace_close_udp_server(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_.template emplace<CloseUdpServerControl<R, W>>(etl::move(p), writable_);
            timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT);
            return etl::move(f);
        }

        inline nb::Poll<void> poll_emplace_send_wifi_frame(WifiFrame &&frame, util::Time &time) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            task_.template emplace<SendWifiFrameControl<R, W>>(writable_, etl::move(frame));
            timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT);
            return nb::ready();
        }

        inline nb::Poll<nb::Future<etl::optional<UdpAddress>>> poll_emplace_get_ip(util::Time &time
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<etl::optional<UdpAddress>>();
            task_.template emplace<GetIpControl<R, W>>(etl::move(p), writable_);
            timeout_ = nb::Delay(time, DEFAULT_TASK_TIMEOUT);
            return etl::move(f);
        }

        void execute(util::Time &time) {
            if (timeout_.has_value() && timeout_->poll(time).is_ready()) {
                LOG_INFO(FLASH_STRING("WIFI TASK TIMEOUT: "), task_.index());
                task_.template emplace<etl::monostate>();
                timeout_ = etl::nullopt;
                return;
            }

            nb::Poll<void> &&poll = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) { return nb::pending; },
                    [&](auto &task) -> nb::Poll<void> { return (task.execute()); },
                },
                task_
            );

            if (poll.is_ready()) {
                task_.template emplace<etl::monostate>();
            }
        }

        void handle_message(WifiMessage<R> &&message) {
            etl::visit(
                util::Visitor{
                    [&](etl::monostate &) {},
                    [&](auto &task) { task.handle_message(etl::move(message)); },
                },
                task_
            );
        }
    };

    template <nb::AsyncReadableWritable R, nb::AsyncWritable W>
    class TaskExecutor {
        nb::Lock<etl::reference_wrapper<memory::Static<R>>> readable_lock_;
        memory::Static<net::link::FrameBroker> &broker_;
        MessageHandler<R> message_handler_{};
        Task<R, W> task_;
        nb::Future<bool> initialization_;

      public:
        TaskExecutor(
            memory::Static<R> &readable,
            memory::Static<W> &writable,
            memory::Static<net::link::FrameBroker> &broker,
            util::Time &time
        )
            : readable_lock_{etl::ref(readable)},
              broker_{broker},
              task_{writable},
              initialization_{etl::move(task_.poll_emplace_initialization(time).unwrap())} {}

        inline nb::Poll<nb::Future<bool>> join_ap(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password,
            util::Time &time
        ) {
            return task_.poll_emplace_join_ap(ssid, password, time);
        }

        inline nb::Poll<nb::Future<bool>> start_udp_server(uint16_t port, util::Time &time) {
            return task_.poll_emplace_start_udp_server(UdpPort{port}, time);
        }

        inline nb::Poll<nb::Future<bool>> close_udp_server(util::Time &time) {
            return task_.poll_emplace_close_udp_server(time);
        }

      private:
        void execute_message_handler(
            net::frame::FrameService &fs,
            LocalServerState &server,
            util ::Time &time
        ) {
            auto &&notification = message_handler_.execute(fs, readable_lock_);
            etl::visit(
                util::Visitor{
                    [&](WifiEvent &&event) {
                        etl::visit(
                            util::Visitor{
                                [&](GotLocalIp &&) {},
                                [&](DisconnectAp &&) { server.on_disconnect_ap(); },
                                [&](ReceiveFrame &&e) {
                                    broker_->poll_dispatch_received_frame(
                                        e.frame.protocol_number, net::link::Address(e.frame.remote),
                                        etl::move(e.frame.reader), time
                                    );
                                },
                            },
                            etl::move(event)
                        );
                    },
                    [&](WifiMessage<R> &&message) { task_.handle_message(etl::move(message)); },
                    [&](etl::monostate &&) {},
                },
                etl::move(notification)
            );
        }

      public:
        void execute(net::frame::FrameService &fs, LocalServerState &server, util::Time &time) {
            execute_message_handler(fs, server, time);
            if (message_handler_.is_exclusive()) {
                return;
            }

            task_.execute(time);
            auto poll = initialization_.poll();
            if ((poll.is_ready() && !poll.unwrap().get()) ||
                initialization_.never_receive_value()) {
                LOG_WARNING(FLASH_STRING("WiFi Initialization failed. Retry."));
                initialization_ = etl::move(task_.poll_emplace_initialization(time).unwrap());
                return;
            }
            if (poll.is_pending()) {
                return;
            }

            while (task_.poll_task_addable().is_ready()) {
                auto &&poll_frame =
                    broker_->poll_get_send_requested_frame(net::link::AddressType::Udp);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                if (!UdpAddress::is_convertible_address(frame.remote)) {
                    continue;
                }

                task_.poll_emplace_send_wifi_frame(
                    WifiFrame::from_link_frame(etl::move(frame)), time
                );
            }
        }
    };
} // namespace media::wifi
