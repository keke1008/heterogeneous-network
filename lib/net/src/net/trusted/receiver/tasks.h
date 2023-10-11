#pragma once

#include "../packet.h"

namespace net::trusted::receiver {
    class WaitingForStartConnectionTask {
        class WaitingForReceivingPacket {};

        etl::variant<WaitingForReceivingPacket, ParsePacketTypeTask> state_;

      public:
        template <frame::IFrameReceiver Receiver>
        nb::Poll<void> execute(Receiver &receiver) {
            if (etl::holds_alternative<WaitingForReceivingPacket>(state_)) {
                auto reader = POLL_UNWRAP_OR_RETURN(receiver.receive_frame());
                state_ = ParsePacketTypeTask{etl::move(reader)};
            }

            if (etl::holds_alternative<ParsePacketTypeTask>(state_)) {
                auto &state = etl::get<ParsePacketTypeTask>(state_);
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

        etl::variant<CreateControlPacketTask<Type>, SendPacketTask, Done> state_{};

      public:
        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        nb::Poll<void> execute(Requester &requester, Sender &sender) {
            if (etl::holds_alternative<CreateControlPacketTask>(state_)) {
                auto &state = etl::get<CreateControlPacketTask>(state_);
                auto reader = POLL_UNWRAP_OR_RETURN(state.execute(requester));
                state_ = SendPacketTask{etl::move(reader)};
            }

            if (etl::holds_alternative<SendPacketTask>(state_)) {
                auto &state = etl::get<SendPacketTask>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                state_ = Done{};
            }

            return nb::ready();
        }
    };

    class ReceivePacketTask {
        etl::variant<WaitingForReceivingPacketTask, ParsePacketTypeTask> state_;

      public:
        ReceivePacketTask(const util::Instant &timeout)
            : state_{WaitingForReceivingPacketTask{timeout}} {}

        template <frame::IFrameReceiver Receiver>
        nb::Poll<etl::pair<PacketType, frame::FrameBufferReader>>
        execute(Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<WaitingForReceivingPacketTask>(state_)) {
                auto &state = etl::get<WaitingForReceivingPacketTask>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (!result.has_value()) {
                    return nb::pending;
                }

                auto &reader = result.value();
                state_ = ParsePacketTypeTask{etl::move(reader)};
            }

            if (etl::holds_alternative<ParsePacketTypeTask>(state_)) {
                auto &state = etl::get<ParsePacketTypeTask>(state_);
                return state.execute(receiver);
            }

            return nb::pending;
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
