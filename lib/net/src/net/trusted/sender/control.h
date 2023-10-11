#pragma once

#include "../common/tasks.h"
#include "../packet.h"

namespace net::trusted {
    template <PacketType Type>
    class SendControlPacket {
        util::Duration timeout_;
        uint8_t remaining_retries_;
        etl::variant<
            common::CreateControlPacketTask<Type>,
            common::SendPacketTask,
            common::ReceivePacketTask>
            state_{};
        etl::optional<frame::FrameBufferReader> transmit_reader_{};

        /**
         * リトライ回数の上限に達した場合，`ready`を返す
         */
        inline nb::Poll<void> retry_send() {
            if (remaining_retries_ -= 1; remaining_retries_ == 0) {
                return nb::ready();
            }
            auto &reader = transmit_reader_.value();
            state_ = common::SendPacketTask{etl::move(reader.make_initial_clone())};
        }

      public:
        SendControlPacket(const util::Duration &timeout, uint8_t retries)
            : timeout_{timeout},
              remaining_retries_{retries} {}

        using Result = etl::expected<void, TrustedError>;

        template <socket::ISenderSocket Sender, socket::IReceiverSocket Receiver>
        nb::Poll<Result> execute(Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<common::CreateControlPacketTask>(state_)) {
                auto &state = etl::get<common::CreateControlPacketTask>(state_);
                auto reader = POLL_UNWRAP_OR_RETURN(state.execute(sender));
                state_ = common::SendPacketTask{etl::move(reader)};
            }

            if (etl::holds_alternative<common::SendPacketTask>(state_)) {
                auto &state = etl::get<common::SendPacketTask>(state_);
                transmit_reader_ = POLL_UNWRAP_OR_RETURN(state.execute(sender));
                state_ = common::WaitingForReceivingPacketTask{time.now() + timeout_};
            }

            if (etl::holds_alternative<common::ReceivePacketTask>(state_)) {
                auto &state = etl::get<common::ReceivePacketTask>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (!result.has_value()) {
                    POLL_UNWRAP_OR_RETURN(retry_send());
                    return Result(etl::unexpected<TrustedError>{TrustedError::Timeout});
                }

                auto [_, packet_type] = result.value();
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
