#pragma once

#include "../packet.h"
#include "./tasks.h"
#include <nb/channel.h>

namespace net::trusted {
    template <socket::ISenderSocket LowerSenderSocket, socket::IReceiverSocket LowerReceiverSocket>
    class ReceiverSocket {
        static constexpr util::Duration TIMEOUT = util::Duration::from_seconds(30);

        LowerSenderSocket sender_;
        LowerReceiverSocket receiver_;
        util::Time &time_;
        etl::optional<frame::FrameBufferReader> received_reader_;

        class Disconnected {};

        etl::variant<
            receiver::WaitingForStartConnectionTask,
            common::ReceivePacketTask,
            receiver::SendControlPacketTask<PacketType::ACK>,
            receiver::SendControlPacketTask<PacketType::NACK>,
            receiver::SendDiconnectionAck,
            Disconnected>
            task_{receiver::WaitingForStartConnectionTask{}};

      public:
        ReceiverSocket() = delete;
        ReceiverSocket(const ReceiverSocket &) = delete;
        ReceiverSocket(ReceiverSocket &&) = default;
        ReceiverSocket &operator=(const ReceiverSocket &) = delete;
        ReceiverSocket &operator=(ReceiverSocket &&) = default;

        explicit ReceiverSocket(
            LowerSenderSocket &&sender,
            LowerReceiverSocket &&receiver,
            util::Time &time
        )
            : sender_{etl::move(sender)},
              receiver_{etl::move(receiver)},
              time_{time} {}

        inline nb::Poll<frame::FrameBufferReader> receive_frame() {
            if (received_reader_.has_value()) {
                auto reader = etl::move(received_reader_.value());
                received_reader_ = etl::nullopt;
                return etl::move(reader);
            } else {
                return nb::pending;
            }
        }

      private:
        nb::Poll<void> execute_inner() {
            if (etl::holds_alternative<receiver::WaitingForStartConnectionTask>(task_)) {
                auto &task = etl::get<receiver::WaitingForStartConnectionTask>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(receiver_));
                task_ = common::ReceivePacketTask{time_.now() + TIMEOUT};
            }

            if (etl::holds_alternative<common::ReceivePacketTask>(task_)) {
                if (received_reader_.has_value()) {
                    return nb::pending;
                }

                auto &task = etl::get<common::ReceivePacketTask>(task_);
                auto result = POLL_UNWRAP_OR_RETURN(task.execute(receiver_, time_));
                if (!result.has_value()) {
                    task_ = Disconnected{};
                    return nb::ready();
                }

                auto [packet_type, reader] = etl::move(result.value());
                if (packet_type == PacketType::DATA) {
                    received_reader_ = etl::move(reader);
                    task_ = receiver::SendControlPacketTask<PacketType::ACK>{};
                } else if (packet_type == PacketType::FIN) {
                    task_ = receiver::SendDiconnectionAck{};
                } else {
                    task_ = common::ReceivePacketTask{time_.now() + TIMEOUT};
                }
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::ACK>>(task_)) {
                auto &task = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(sender_));
                task_ = common::ReceivePacketTask{time_.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::NACK>>(task_)) {
                auto &task = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(sender_));
                task_ = common::ReceivePacketTask{time_.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::SendDiconnectionAck>(task_)) {
                auto &task = etl::get<receiver::SendDiconnectionAck>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(sender_));
                task_ = Disconnected{};
                return nb::ready();
            }

            if (etl::holds_alternative<Disconnected>(task_)) {
                return nb::ready();
            }

            return nb::pending;
        }

      public:
        nb::Poll<void> execute() {
            if (receiver_.execute().is_ready()) {
                return nb::ready();
            }
            auto result = execute_inner();
            if (sender_.execute().is_ready()) {
                return nb::ready();
            }
            return result;
        }
    };
} // namespace net::trusted
