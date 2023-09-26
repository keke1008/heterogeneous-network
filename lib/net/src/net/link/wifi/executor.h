#pragma once

#include "../frame.h"
#include "control.h"
#include "message.h"
#include <etl/optional.h>
#include <etl/variant.h>
#include <nb/future.h>
#include <nb/stream.h>
#include <util/non_copyable.h>
#include <util/visitor.h>

namespace net::link::wifi {
    namespace {
        using Task = etl::
            variant<Initialization, JoinAp, StartUdpServer, SendData, ReceiveDataMessageHandler>;

        using NonCopyableTask = util::NonCopyable<Task>;

    } // namespace

    class WifiExecutor {
        etl::variant<etl::monostate, MessageDetector, NonCopyableTask> buffer_;
        nb::stream::ReadableWritableStream &stream_;

        nb::Poll<void> wait_until_task_addable() const {
            bool task_addable =
                etl::holds_alternative<etl::monostate>(buffer_) // 受信したデータを持っていない
                && stream_.readable_count() == 0;               // 何も受信していない
            return task_addable ? nb::ready() : nb::pending;
        }

      public:
        WifiExecutor(nb::stream::ReadableWritableStream &stream) : stream_{stream} {}

        nb::Poll<nb::Future<bool>> initialize() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = Initialization{etl::move(promise)};
            buffer_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<nb::Future<bool>>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = JoinAp{etl::move(promise), ssid, password};
            buffer_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<nb::Future<bool>> start_udp_server(uint16_t port) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = StartUdpServer{etl::move(promise), port};
            buffer_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<etl::pair<nb::Future<DataWriter>, nb::Future<bool>>>
        send_data(uint8_t length, IPv4Address &remote_address, uint16_t remote_port) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [body_future, body_promise] = nb::make_future_promise_pair<DataWriter>();
            auto [result_future, result_promise] = nb::make_future_promise_pair<bool>();
            auto task = SendData{
                etl::move(body_promise),
                etl::move(result_promise),
                length,
                remote_address,
                remote_port,
            };
            buffer_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::make_pair(etl::move(body_future), etl::move(result_future)));
        }

      private:
        nb::Poll<void> handle_task() {
            auto &task = etl::get<NonCopyableTask>(buffer_);
            return etl::visit(
                [&](auto &task) -> nb::Poll<void> {
                    POLL_UNWRAP_OR_RETURN(task.execute(stream_));
                    buffer_.emplace<etl::monostate>();
                    return nb::ready();
                },
                task.get()
            );
        }

      public:
        nb::Poll<FrameReception> execute() {
            if (etl::holds_alternative<etl::monostate>(buffer_)) {
                if (stream_.readable_count() == 0) {
                    return nb::pending;
                }
                buffer_ = MessageDetector{};
            }

            if (etl::holds_alternative<MessageDetector>(buffer_)) {
                auto &message_detector = etl::get<MessageDetector>(buffer_);
                auto message_type = POLL_UNWRAP_OR_RETURN(message_detector.execute(stream_));
                switch (message_type) {
                case MessageType::DataReceived: {
                    auto [body_future, body_promise] = nb::make_future_promise_pair<DataReader>();
                    auto [source_future, source_promise] = nb::make_future_promise_pair<Address>();
                    auto task = ReceiveDataMessageHandler{
                        etl::move(body_promise), etl::move(source_promise)};
                    buffer_ = etl::move(NonCopyableTask{etl::move(task)});
                    handle_task();

                    return nb::ready(FrameReception{
                        .body = etl::move(body_future),
                        .source = etl::move(source_future),
                    });
                }
                case MessageType::Unknown: {
                    buffer_ = etl::monostate{};
                    return nb::pending;
                }
                default: {
                    DEBUG_ASSERT(false, "Unreachable");
                }
                }
            }

            if (etl::holds_alternative<NonCopyableTask>(buffer_)) {
                POLL_UNWRAP_OR_RETURN(handle_task());
                return nb::pending;
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
