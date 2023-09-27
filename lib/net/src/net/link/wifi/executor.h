#pragma once

#include "../frame.h"
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
        using Task = etl::
            variant<Initialization, JoinAp, StartUdpServer, SendData, ReceiveDataMessageHandler>;

        using NonCopyableTask = util::NonCopyable<Task>;

    } // namespace

    class WifiExecutor {
        uint16_t port_number_;

        etl::variant<etl::monostate, MessageDetector, NonCopyableTask> buffer_;
        nb::stream::ReadableWritableStream &stream_;

        nb::Poll<void> wait_until_task_addable() const {
            bool task_addable =
                etl::holds_alternative<etl::monostate>(buffer_) // 受信したデータを持っていない
                && stream_.readable_count() == 0;               // 何も受信していない
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

        nb::Poll<nb::Future<bool>> start_udp_server() {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = StartUdpServer{etl::move(promise), port_number_};
            buffer_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<FrameTransmission> send_data(const Address &destination, uint8_t length) {
            DEBUG_ASSERT(destination.type() == AddressType::IPv4);
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto remote_address = IPv4Address{destination};
            auto [frame, p_body, p_success] = FrameTransmission::make_frame_transmission();
            buffer_ = etl::move(NonCopyableTask{SendData{
                etl::move(p_body), etl::move(p_success), length, remote_address, port_number_}});
            return nb::ready(etl::move(frame));
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
                    auto [frame, p_body, p_source] = FrameReception::make_frame_reception();
                    auto task = ReceiveDataMessageHandler{etl::move(p_body), etl::move(p_source)};
                    buffer_ = etl::move(NonCopyableTask{etl::move(task)});
                    handle_task();
                    return nb::ready(etl::move(frame));
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
