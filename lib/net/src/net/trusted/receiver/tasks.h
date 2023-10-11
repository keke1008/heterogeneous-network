#pragma once

#include "../common/tasks.h"

namespace net::trusted::receiver {
    class WaitingForStartConnectionTask {
        class WaitingForReceivingPacket {};

        etl::variant<WaitingForReceivingPacket, common::ParsePacketTypeTask> state_;

      public:
        template <frame::IFrameReceiver Receiver>
        nb::Poll<void> execute(Receiver &receiver) {
            if (etl::holds_alternative<WaitingForReceivingPacket>(state_)) {
                auto reader = POLL_UNWRAP_OR_RETURN(receiver.receive_frame());
                state_ = common::ParsePacketTypeTask{etl::move(reader)};
            }

            if (etl::holds_alternative<common::ParsePacketTypeTask>(state_)) {
                auto &state = etl::get<common::ParsePacketTypeTask>(state_);
                auto [type, reader] = POLL_UNWRAP_OR_RETURN(state.execute(receiver));
                if (type == PacketType::SYN) {
                    return nb::ready();
                } else {
                    state_ = WaitingForReceivingPacket{};
                }
            }
            return nb::pending;
        }
    };

    template <PacketType Type>
    class SendControlPacketTask {
        class Done {};

        etl::variant<common::CreateControlPacketTask<Type>, common::SendPacketTask, Done> state_{};

      public:
        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        nb::Poll<void> execute(Requester &requester, Sender &sender) {
            if (etl::holds_alternative<common::CreateControlPacketTask>(state_)) {
                auto &state = etl::get<common::CreateControlPacketTask>(state_);
                auto reader = POLL_UNWRAP_OR_RETURN(state.execute(requester));
                state_ = common::SendPacketTask{etl::move(reader)};
            }

            if (etl::holds_alternative<common::SendPacketTask>(state_)) {
                auto &state = etl::get<common::SendPacketTask>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                state_ = Done{};
            }

            return nb::ready();
        }
    };

    class SendDiconnectionAck {
        SendControlPacketTask<PacketType::ACK> task_;

      public:
        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        inline nb::Poll<void> execute(Requester &requester, Sender &sender) {
            return task_.execute(requester, sender);
        }
    };
} // namespace net::trusted::receiver
