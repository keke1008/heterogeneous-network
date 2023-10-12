#pragma once

#include "../address.h"
#include "control.h"
#include "message.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/variant.h>
#include <nb/future.h>
#include <nb/stream.h>
#include <util/non_copyable.h>
#include <util/visitor.h>

namespace net::link::wifi {
    namespace {
        using Task = etl::variant<
            etl::monostate,
            Initialization,
            JoinAp,
            StartUdpServer,
            SendData,
            MessageHandler>;

        using NonCopyableTask = util::NonCopyable<Task>;
    } // namespace

    class WifiExecutor {
        uint16_t port_number_;
        etl::optional<IPv4Address> self_address_;
        etl::optional<Frame> received_frame_;

        NonCopyableTask task_;
        nb::stream::ReadableWritableStream &stream_;

        inline nb::Poll<void> wait_until_task_addable() const {
            bool task_addable =
                etl::holds_alternative<etl::monostate>(task_.get()) // タスクを持っていない
                && stream_.readable_count() == 0;                   // 何も受信していない
            return task_addable ? nb::ready() : nb::pending;
        }

      public:
        WifiExecutor() = delete;
        WifiExecutor(const WifiExecutor &) = delete;
        WifiExecutor(WifiExecutor &&) = default;
        WifiExecutor &operator=(const WifiExecutor &) = delete;
        WifiExecutor &operator=(WifiExecutor &&) = delete;

        WifiExecutor(nb::stream::ReadableWritableStream &stream, uint16_t port_number)
            : port_number_{port_number},
              stream_{stream} {}

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::IPv4;
        }

        nb::Poll<nb::Future<bool>> initialize() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = Initialization{etl::move(promise)};
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<nb::Future<bool>>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = JoinAp{etl::move(promise), ssid, password};
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<nb::Future<bool>> start_udp_server() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = StartUdpServer{etl::move(promise), port_number_};
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<void> send_frame(Frame &&frame) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());
            auto task = SendData{etl::move(frame), port_number_};
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready();
        }

        nb::Poll<Frame> receive_frame() {
            if (received_frame_.has_value()) {
                auto frame = etl::move(received_frame_.value());
                received_frame_.reset();
                return etl::move(frame);
            } else {
                return nb::pending;
            }
        }

      public:
        template <net::frame::IFrameService FrameService>
        void execute(FrameService &service) {
            if (etl::holds_alternative<etl::monostate>(task_.get())) {
                if (stream_.readable_count() != 0) {
                    bool discard_frame = received_frame_.has_value();
                    task_.get() = MessageHandler{Address{self_address_.value()}, discard_frame};
                } else {
                    return;
                }
            }

            if (etl::holds_alternative<MessageHandler>(task_.get())) {
                auto &task = etl::get<MessageHandler>(task_.get());
                auto opt_frame = task.execute(service, stream_);
                if (opt_frame.is_pending()) {
                    return;
                }

                task_.get().emplace<etl::monostate>();
                if (opt_frame.has_value()) {
                    received_frame_ = opt_frame.value();
                }
            }

            // その他のタスク
            auto poll = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) { return nb::pending; },
                    [&](MessageHandler &) { return nb::pending; },
                    [&](ReceiveDataMessageHandler &task) {
                        received_frame_ =
                            POLL_MOVE_UNWRAP_OR_RETURN(task.execute(service, stream_));
                        return nb::ready();
                    },
                    [&](auto &task) -> nb::Poll<void> { return task.execute(stream_); },
                },
                task_.get()
            );
            if (poll.is_ready()) {
                task_.get().emplace<etl::monostate>();
            }
        }
    };
} // namespace net::link::wifi
