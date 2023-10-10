#pragma once

#include "../packet.h"
#include <memory/pair_shared.h>

namespace net::trusted {
    class SendSingleDataPacket {
        etl::variant<SendPacketTask, WaitingForReceivingPacketTask, ParsePacketTypeTask> state_;
        frame::FrameBufferReader transmit_reader_;
        util::Duration timeout_;
        uint8_t remaining_retries_;

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
            : state_{SendPacketTask{reader.make_initial_clone()}},
              transmit_reader_{etl::move(reader)},
              timeout_{timeout},
              remaining_retries_{retries} {}

        using Result = etl::expected<void, TrustedError>;

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<Result>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<SendPacketTask>(state_)) {
                auto &state = etl::get<SendPacketTask>(state_);
                transmit_reader_ = POLL_UNWRAP_OR_RETURN(state.execute(requester, sender));
                state_ = WaitingForReceivingPacketTask{time.now() + timeout_};
            }

            if (etl::holds_alternative<WaitingForReceivingPacketTask>(state_)) {
                auto &state = etl::get<WaitingForReceivingPacketTask>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (!result.has_value()) {
                    return Result(etl::unexpected<TrustedError>{TrustedError::Timeout});
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
                    if (remaining_retries_ -= 1; remaining_retries_ == 0) {
                        return Result(etl::unexpected<TrustedError>{TrustedError::BadNetwork});
                    }
                    state_ = SendPacketTask{etl::move(transmit_reader_.make_initial_clone())};
                }
            }

            return nb::pending;
        }
    };

    class SendDataPacket {
        memory::Owned<etl::optional<frame::FrameBufferReader>> transmit_reader_;
        etl::optional<SendSingleDataPacket> send_packet_;
        util::Duration timeout_;
        uint8_t retries_;

      public:
        explicit SendDataPacket(
            memory::Owned<etl::optional<frame::FrameBufferReader>> &&transmit_reader,
            util::Duration timeout,
            uint8_t retries
        )
            : transmit_reader_{etl::move(transmit_reader)},
              timeout_{timeout},
              retries_{retries} {}

        class CloseConnectionRequested {};

        using Result = etl::expected<CloseConnectionRequested, TrustedError>;

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<Result>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (send_packet_.has_value()) {
                auto result =
                    POLL_UNWRAP_OR_RETURN(send_packet_->execute(requester, sender, receiver, time));
                if (result.has_value()) {
                    send_packet_ = etl::nullopt;
                } else {
                    return Result(etl::unexpected<TrustedError>{result.error()});
                }
            }

            if (!transmit_reader_.has_pair()) {
                return Result{};
            }

            auto &reader = transmit_reader_.get();
            if (reader.has_value()) {
                send_packet_ = SendSingleDataPacket{
                    etl::move(reader.value()),
                    timeout_,
                    retries_,
                };
                reader = etl::nullopt;
            }
        }
    };
} // namespace net::trusted
