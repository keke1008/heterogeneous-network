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
            GetIp,
            MessageHandler>;

        using NonCopyableTask = util::NonCopyable<Task>;
    } // namespace

    class WifiExecutor {
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

        WifiExecutor(nb::stream::ReadableWritableStream &stream) : stream_{stream} {}

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::IPv4;
        }

        inline etl::optional<Address> get_address() const {
            return self_address_.has_value() ? etl::optional(Address{self_address_.value()})
                                             : etl::nullopt;
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

        nb::Poll<nb::Future<bool>> start_udp_server(uint16_t port_number) {
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto [future, promise] = nb::make_future_promise_pair<bool>();
            auto task = StartUdpServer{etl::move(promise), port_number};
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready(etl::move(future));
        }

        nb::Poll<void> send_frame(SendingFrame &frame) {
            DEBUG_ASSERT(frame.peer.type() == AddressType::IPv4);
            POLL_UNWRAP_OR_RETURN(wait_until_task_addable());

            auto task = SendData{
                frame.protocol_number,
                IPv4Address(frame.peer),
                etl::move(frame.reader_ref),
            };
            task_ = etl::move(NonCopyableTask{etl::move(task)});
            return nb::ready();
        }

        nb::Poll<Frame> receive_frame(frame::ProtocolNumber protocol_number) {
            if (received_frame_.has_value()) {
                auto &ref_frame = received_frame_.value();
                if (ref_frame.protocol_number != protocol_number) {
                    return nb::pending;
                }

                auto frame = etl::move(ref_frame);
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
                    task_.get() = MessageHandler{discard_frame};
                } else {
                    return;
                }
            }

            if (etl::holds_alternative<MessageHandler>(task_.get())) {
                auto &task = etl::get<MessageHandler>(task_.get());
                auto poll_result = task.execute(service, stream_);
                if (poll_result.is_pending()) {
                    return;
                }

                task_.get().emplace<etl::monostate>();
                etl::visit(
                    util::Visitor{
                        [&](etl::monostate &) {},
                        [&](Frame &frame) { received_frame_ = etl::move(frame); },
                        [&](WifiEvent &event) {
                            switch (event) {
                            case WifiEvent::GotIp: {
                                task_.get().emplace<GetIp>();
                            }
                            case WifiEvent::Disconnect: {
                                self_address_ = etl::nullopt;
                            }
                            }
                        },
                    },
                    poll_result.unwrap()
                );
                if (poll_result.unwrap().has_value()) {
                    received_frame_ = etl::move(poll_result.unwrap().value());
                }
            }

            // その他のタスク
            auto poll = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) -> nb::Poll<void> { return nb::pending; },
                    [&](MessageHandler &) -> nb::Poll<void> { return nb::pending; },
                    [&](GetIp &task) -> nb::Poll<void> {
                        const auto &address = POLL_UNWRAP_OR_RETURN(task.execute(stream_));
                        if (address.has_value()) {
                            self_address_ = address.value();
                        }
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
