#pragma once

#include "../packet.h"
#include "./packet.h"
#include "./tasks.h"
#include <memory/pair_shared.h>

namespace net::trusted {
    class ReceiverSocket {
        static constexpr util::Duration TIMEOUT = util::Duration::from_seconds(30);

        memory::Reference<etl::optional<frame::FrameBufferReader>> received_reader_;
        etl::variant<
            receiver::WaitingForStartConnectionTask,
            receiver::ReceivePacketTask,
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

        explicit ReceiverSocket(
            memory::Reference<etl::optional<frame::FrameBufferReader>> &&received_reader
        )
            : received_reader_{etl::move(received_reader)} {}

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<void>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<receiver::WaitingForStartConnectionTask>(task_)) {
                auto &task = etl::get<receiver::WaitingForStartConnectionTask>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(receiver));
                task_ = receiver::ReceivePacketTask{time.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::ReceivePacketTask>(task_)) {
                if (!received_reader_.has_pair() || received_reader_.get().has_value()) {
                    return nb::pending;
                }

                auto &state = etl::get<receiver::ReceivePacketTask>(task_);
                auto [packet_type, reader] = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (packet_type == PacketType::DATA) {
                    task_ = receiver::SendControlPacketTask<PacketType::ACK>{};
                    received_reader_ = etl::move(reader);
                } else if (packet_type == PacketType::FIN) {
                    task_ = receiver::SendDiconnectionAck{};
                } else {
                    task_ = receiver::ReceivePacketTask{time.now() + TIMEOUT};
                }
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::ACK>>(task_)) {
                auto &state = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                task_ = receiver::ReceivePacketTask{time.now() + TIMEOUT};
            }

            if (etl::holds_alternative<receiver::SendControlPacketTask<PacketType::NACK>>(task_)) {
                auto &state = etl::get<receiver::SendControlPacketTask<PacketType::ACK>>(task_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                task_ = receiver::ReceivePacketTask{time.now() + TIMEOUT};
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
        auto [owned, reference] =
            memory::make_shared<etl::optional<frame::FrameBufferReader>>(etl::nullopt);
        return etl::pair<ReceiverSocket, receiver::DataPacketReceiver>{
            ReceiverSocket{etl::move(reference)},
            receiver::DataPacketReceiver{etl::move(owned)},
        };
    }
} // namespace net::trusted
