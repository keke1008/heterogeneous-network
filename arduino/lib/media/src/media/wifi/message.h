#pragma once

#include "./message/notification.h" // IWYU pragma: export
#include "./message/packet_received.h"
#include "./message/receiver.h"
#include "./message/unknown.h"
#include "./message/wifi.h"

namespace media::wifi {
    template <nb::AsyncReadable R>
    class MessageHandler {
        etl::variant<
            etl::monostate,
            MessageReceiver<R>,
            UnknownMessageHandler<R>,
            PacketReceivedMessageHandler<R>,
            WifiMessageHandler<R>>
            task_;

      public:
        inline bool is_exclusive() const {
            return !etl::holds_alternative<etl::monostate>(task_);
        }

        etl::variant<etl::monostate, WifiEvent, WifiMessage<R>>
        execute(nb::Lock<etl::reference_wrapper<R>> &readable) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                if (auto r = readable.poll_lock(); r.is_ready()) {
                    auto &readable = r.unwrap();
                    if (readable.get().poll_readable(1).is_ready()) {
                        task_ = MessageReceiver<R>{etl::move(readable)};
                    }
                }
            }

            if (etl::holds_alternative<MessageReceiver<R>>(task_)) {
                MessageReceiver<R> &receiver = etl::get<MessageReceiver<R>>(task_);
                nb::Poll<EspATMessage<R>> &&poll_message = receiver.execute(readable);
                if (poll_message.is_pending()) {
                    return etl::nullopt;
                }

                task_.template emplace<etl::monostate>();

                EspATMessage<R> &&message = poll_message.unwrap();
                switch (message.type) {
                case MessageType::UnknownHeader:
                    task_ = UnknownMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::UnknownHeaderEndWithCR:
                    task_ = UnknownMessageHandler{is_last_cr, etl::move(message.body)};
                    break;
                case MessageType::UnknownMessage:
                    return etl::nullopt;
                case MessageType::IPDHeader:
                    task_ = PacketReceivedMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::WifiHeader:
                    task_ = WifiMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::Ok:
                    return WifiMessage<R>{WifiResponseMessage::Ok};
                case MessageType::Error:
                    return WifiMessage<R>{WifiResponseMessage::Error};
                case MessageType::Fail:
                    return WifiMessage<R>{WifiResponseMessage::Fail};
                case MessageType::SendOk:
                    return WifiMessage<R>{WifiResponseMessage::SendOk};
                case MessageType::SendFail:
                    return WifiMessage<R>{WifiResponseMessage::SendFail};
                case MessageType::SendPrompt:
                    return WifiMessage<R>{WifiResponseMessage::SendPrompt};
                default:
                    UNREACHABLE_DEFAULT_CASE;
                }
            }

            if (etl::holds_alternative<UnknownMessageHandler<R>>(task_)) {
                UnknownMessageHandler<R> &unknown = etl::get<UnknownMessageHandler<R>>(task_);
                if (unknown.execute().is_ready()) {
                    task_ = MessageReceiver<R>{};
                }
            }

            if (etl::holds_alternative<PacketReceivedMessageHandler<R>>(task_)) {
                PacketReceivedMessageHandler<R> &packet_received =
                    etl::get<PacketReceivedMessageHandler<R>>(task_);
                nb::Poll<etl::optional<WifiEvent>> &&poll_opt_event = packet_received.execute();
                if (poll_opt_event.is_pending()) {
                    return etl::nullopt;
                }

                task_ = MessageReceiver<R>{};
                return poll_opt_event.unwrap();
            }

            if (etl::holds_alternative<WifiMessageHandler<R>>(task_)) {
                WifiMessageHandler<R> &wifi = etl::get<WifiMessageHandler<R>>(task_);
                nb::Poll<etl::optional<WifiEvent>> &&poll_opt_event = wifi.execute();
                if (poll_opt_event.is_pending()) {
                    return etl::nullopt;
                }

                task_ = MessageReceiver<R>{};
                return poll_opt_event.unwrap();
            }
        }
    };
} // namespace media::wifi
