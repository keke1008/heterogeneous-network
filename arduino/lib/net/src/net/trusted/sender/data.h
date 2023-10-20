#pragma once

#include "../common/tasks.h"
#include "../packet.h"

namespace net::trusted {
    class SendSingleDataPacket {
        etl::variant<common::SendPacketTask, common::ReceivePacketTask> state_;
        frame::FrameBufferReader transmit_reader_;
        util::Duration timeout_;
        uint8_t remaining_retries_;

        /**
         * リトライ回数の上限に達した場合，`ready`を返す
         */
        inline nb::Poll<void> retry_send() {
            if (remaining_retries_--; remaining_retries_ == 0) {
                return nb::ready();
            }
            state_ = common::SendPacketTask{etl::move(transmit_reader_.make_initial_clone())};
            return nb::pending;
        }

      public:
        SendSingleDataPacket() = delete;
        SendSingleDataPacket(const SendSingleDataPacket &) = delete;
        SendSingleDataPacket(SendSingleDataPacket &&) = default;
        SendSingleDataPacket &operator=(const SendSingleDataPacket &) = delete;
        SendSingleDataPacket &operator=(SendSingleDataPacket &&) = default;

        explicit SendSingleDataPacket(
            frame::FrameBufferReader &&reader,
            util::Duration timeout,
            uint8_t retries
        )
            : state_{common::SendPacketTask{reader.make_initial_clone()}},
              transmit_reader_{etl::move(reader)},
              timeout_{timeout},
              remaining_retries_{retries} {}

        using Result = etl::expected<void, TrustedError>;

        template <socket::ISenderSocket Sender, socket::IReceiverSocket Receiver>
        nb::Poll<Result> execute(Sender &sender, Receiver &receiver, util::Time &time) {
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
                    POLL_UNWRAP_OR_RETURN(retry_send());
                    return Result(etl::unexpected<TrustedError>{TrustedError::BadNetwork});
                }
            }

            return nb::pending;
        }
    };

    class SendDataPacket {
        etl::optional<SendSingleDataPacket> send_packet_;
        util::Duration timeout_;
        uint8_t retries_;

      public:
        explicit SendDataPacket(util::Duration timeout, uint8_t retries)
            : timeout_{timeout},
              retries_{retries} {}

        inline nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            if (send_packet_.has_value()) {
                return nb::pending;
            } else {
                send_packet_ = SendSingleDataPacket{
                    etl::move(reader),
                    timeout_,
                    retries_,
                };
                return nb::ready();
            }
        }

        class CloseConnectionRequested {};

        using Result = etl::expected<CloseConnectionRequested, TrustedError>;

        template <socket::ISenderSocket Sender, socket::IReceiverSocket Receiver>
        nb::Poll<Result> execute(Sender &sender, Receiver &receiver, util::Time &time) {
            if (send_packet_.has_value()) {
                auto result = POLL_UNWRAP_OR_RETURN(send_packet_->execute(sender, receiver, time));
                if (result.has_value()) {
                    send_packet_ = etl::nullopt;
                } else {
                    return Result(etl::unexpected<TrustedError>{result.error()});
                }
            }
        }
    };
} // namespace net::trusted
