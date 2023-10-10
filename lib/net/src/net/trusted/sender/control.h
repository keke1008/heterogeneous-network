#pragma once

#include "../packet.h"

namespace net::trusted {
    template <PacketType Type>
    class SendControlPacket {
        util::Duration timeout_;
        uint8_t remaining_retries_;
        etl::variant<
            CreateControlPacketTask<Type>,
            SendPacketTask,
            WaitingForReceivingPacketTask,
            ParsePacketTypeTask>
            state_{};
        etl::optional<frame::FrameBufferReader> transmit_reader_{};

      public:
        SendControlPacket(const util::Duration &timeout, uint8_t retries)
            : timeout_{timeout},
              remaining_retries_{retries} {}

        using Result = etl::expected<void, TrustedError>;

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<Result>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<CreateControlPacketTask>(state_)) {
                auto &state = etl::get<CreateControlPacketTask>(state_);
                auto reader = POLL_UNWRAP_OR_RETURN(state.execute(requester));
                state_ = SendPacketTask{etl::move(reader)};
            }

            if (etl::holds_alternative<SendPacketTask>(state_)) {
                auto &state = etl::get<SendPacketTask>(state_);
                transmit_reader_ = POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                state_ = WaitingForReceivingPacketTask{time.now() + timeout_};
            }

            if (etl::holds_alternative<WaitingForReceivingPacketTask>(state_)) {
                auto &state = etl::get<WaitingForReceivingPacketTask>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (!result.has_value()) {
                    if (remaining_retries_ -= 1; remaining_retries_ == 0) {
                        return Result(etl::unexpected<TrustedError>{TrustedError::Timeout});
                    }
                    state_ =
                        SendPacketTask{etl::move(transmit_reader_.value().make_initial_clone())};
                    return nb::pending;
                }

                auto &reader = result.value();
                state_ = ParsePacketTypeTask{etl::move(reader)};
            }

            if (etl::holds_alternative<ParsePacketTypeTask>(state_)) {
                auto &state = etl::get<ParsePacketTypeTask>(state_);
                auto packet_type = POLL_UNWRAP_OR_RETURN(state.execute(receiver));
                if (packet_type == PacketType::ACK) {
                    return Result{};
                } else if (packet_type == PacketType::NACK) {
                    return Result(etl::unexpected<TrustedError>{TrustedError::ConnectionRefused});
                }
            }

            return nb::pending;
        }
    };

} // namespace net::trusted
