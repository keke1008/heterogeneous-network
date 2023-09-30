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

      private:
        template <net::frame::IFrameService<Address> FrameService>
        void handle_monostate(FrameService &service) {
            if (stream_.readable_count() != 0) {
                buffer_ = MessageDetector{};
                return;
            }

            auto poll_request = service.poll_transmission_request([](auto &request) {
                return request.destination.type() == AddressType::IPv4;
            });
            if (poll_request.is_ready()) {
                auto task = SendData{
                    etl::move(poll_request.unwrap()),
                    port_number_,
                };
                buffer_ = etl::move(NonCopyableTask{etl::move(task)});
                handle_task(service);
            }
        };

        template <net::frame::IFrameService<Address> FrameService>
        void handle_task(FrameService &service) {
            auto &task = etl::get<NonCopyableTask>(buffer_);
            auto poll = etl::visit(
                util::Visitor{
                    [&](ReceiveDataMessageHandler &task) {
                        return (task.execute(service, stream_));
                    },
                    [&](auto &task) -> nb::Poll<void> { return task.execute(stream_); },
                },
                task.get()
            );

            if (poll.is_ready()) {
                buffer_.emplace<etl::monostate>();
            }
        }

      public:
        template <net::frame::IFrameService<Address> FrameService>
        void execute(FrameService &service) {
            if (etl::holds_alternative<etl::monostate>(buffer_)) {
                handle_monostate(service);
            }
            if (etl::holds_alternative<etl::monostate>(buffer_)) {
                return;
            }

            if (etl::holds_alternative<MessageDetector>(buffer_)) {
                auto &message_detector = etl::get<MessageDetector>(buffer_);
                auto poll_message_type = message_detector.execute(stream_);
                if (poll_message_type.is_pending()) {
                    return;
                }

                switch (poll_message_type.unwrap()) {
                case MessageType::DataReceived: {
                    buffer_ = etl::move(NonCopyableTask{ReceiveDataMessageHandler{}});
                    handle_task(service);
                    return;
                }
                case MessageType::Unknown: {
                    buffer_ = etl::monostate{};
                    return;
                }
                default: {
                    DEBUG_ASSERT(false, "Unreachable");
                }
                }
            }

            if (etl::holds_alternative<NonCopyableTask>(buffer_)) {
                handle_task(service);
            }
        }
    };
} // namespace net::link::wifi
