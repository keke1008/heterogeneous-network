#pragma once

#include "../packet.h"
#include "./packet.h"
#include "./tasks.h"
#include <nb/channel.h>

namespace net::trusted {
    class ReceiverSocket {
        static constexpr util::Duration TIMEOUT = util::Duration::from_seconds(30);

        nb::OneBufferSender<frame::FrameBufferReader> reader_tx_;
        etl::variant<
            receiver::WaitingForStartConnectionTask,
            common::ReceivePacketTask,
            receiver::SendControlPacketTask<PacketType::ACK>,
            receiver::SendControlPacketTask<PacketType::NACK>,
            receiver::SendDiconnectionAck>
            task_{receiver::WaitingForStartConnectionTask{}};

      public:
        ReceiverSocket() = delete;
        ReceiverSocket(const ReceiverSocket &) = delete;
        ReceiverSocket(ReceiverSocket &&) = default;
        ReceiverSocket &operator=(const ReceiverSocket &) = delete;
        ReceiverSocket &operator=(ReceiverSocket &&) = default;

        explicit ReceiverSocket(nb::OneBufferSender<frame::FrameBufferReader> reader_tx)
            : reader_tx_{etl::move(reader_tx)} {}

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<void>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<receiver::WaitingForStartConnectionTask>(task_)) {
                auto &task = etl::get<receiver::WaitingForStartConnectionTask>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(receiver));
                task_ = common::ReceivePacketTask{time.now() + TIMEOUT};
            }

            if (etl::holds_alternative<common::ReceivePacketTask>(task_)) {
                POLL_UNWRAP_OR_RETURN(reader_tx_.poll_sendable());
                auto &state = etl::get<common::ReceivePacketTask>(task_);
                auto [packet_type, reader] = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (packet_type == PacketType::DATA) {
                    task_ = receiver::SendControlPacketTask<PacketType::ACK>{};
                    reader_tx_.send(etl::move(reader));
                } else if (packet_type == PacketType::FIN) {
                    task_ = receiver::SendDiconnectionAck{};
                } else {
                    task_ = common::ReceivePacketTask{time.now() + TIMEOUT};
                }
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::ACK>>(task_)) {
                auto &state = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                task_ = common::ReceivePacketTask{time.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::NACK>>(task_)) {
                auto &state = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                task_ = common::ReceivePacketTask{time.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::SendDiconnectionAck>(task_)) {
                auto &state = etl::get<receiver::SendDiconnectionAck>(task_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                return nb::ready();
            }

            return nb::pending;
        }
    };

    inline etl::pair<ReceiverSocket, receiver::DataPacketReceiver> make_receiver_socket() {
        auto [tx, rx] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return etl::make_pair(
            ReceiverSocket{etl::move(tx)}, receiver::DataPacketReceiver{etl::move(rx)}
        );
    }
} // namespace net::trusted
