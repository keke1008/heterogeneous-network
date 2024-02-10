#pragma once

#include "./message/packet_received.h"
#include "./message/receiver.h"
#include "./message/unknown.h"
#include "./message/wifi.h"

namespace media::wifi {
    template <nb::AsyncReadable R>
    class MessageHandler {
        etl::variant<
            MessageReceiver<R>,
            UnknownMessageHandler<R>,
            PacketReceivedMessageHandler<R>,
            WifiMessageHandler<R>>
            task_;

      public:
        inline bool is_exclusive() const {
            return !etl::holds_alternative<MessageReceiver<R>>(task_);
        }

        inline etl::optional<WifiEvent> execute(nb::Lock<etl::reference_wrapper<R>> r) {
            if (etl::holds_alternative<MessageReceiver<R>>(task_)) {
                MessageReceiver<R> &receiver = etl::get<MessageReceiver<R>>(task_);
                etl::optional<Message<R>> &&opt_message = receiver.execute(r);
                if (!opt_message.has_value()) {
                    return etl::nullopt;
                }

                Message<R> &&message = opt_message.value();
                switch (message.type) {
                case MessageType::UnknownHeader:
                    task_ = UnknownMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::UnknownHeaderEndWithCR:
                    task_ = UnknownMessageHandler{is_last_cr, etl::move(message.body)};
                    break;
                case MessageType::UnknownMessage:
                    return etl::nullopt;
                case MessageType::PacketReceivedHeader:
                    task_ = PacketReceivedMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::WifiHeader:
                    task_ = WifiMessageHandler{etl::move(message.body)};
                    break;
                case MessageType::Ok:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::OK}};
                case MessageType::Error:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::Error}};
                case MessageType::Fail:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::Fail}};
                case MessageType::SendOk:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::SendOk}};
                case MessageType::SendFail:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::SendFail}};
                case MessageType::SendPrompt:
                    return WifiEvent{ReceivePassiveMessage{PassiveMessage::SendPrompt}};
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
