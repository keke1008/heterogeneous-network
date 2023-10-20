#pragma once

#include "../packet.h"
#include "./control.h"
#include "./data.h"
#include <net/link.h>

namespace net::trusted {
    template <socket::ISenderSocket LowerSenderSocket, socket::IReceiverSocket LowerReceiverSocket>
    class SenderSocket {
        static constexpr uint8_t MAX_RETRIES = 3;
        static constexpr util::Duration TIMEOUT = util::Duration::from_millis(1000);

        using State = etl::variant<
            SendControlPacket<PacketType::SYN>,
            SendDataPacket,
            SendControlPacket<PacketType::FIN>>;

        LowerSenderSocket sender_;
        LowerReceiverSocket receiver_;
        util::Time &time_;
        State state_{SendControlPacket<PacketType::SYN>{TIMEOUT, MAX_RETRIES}};

      public:
        SenderSocket() = delete;
        SenderSocket(const SenderSocket &) = delete;
        SenderSocket(SenderSocket &&) = default;
        SenderSocket &operator=(const SenderSocket &) = delete;
        SenderSocket &operator=(SenderSocket &&) = default;

        explicit SenderSocket(
            LowerSenderSocket &&sender,
            LowerReceiverSocket &&receiver,
            util::Time &time
        )
            : sender_{etl::move(sender)},
              receiver_{etl::move(receiver)},
              time_{time} {}

      public:
        nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            auto writer = POLL_UNWRAP_OR_RETURN(sender_.requset_frame_writer(length));
            writer.build(PacketHeaderWriter{PacketType::DATA});
            return etl::move(writer);
        }

        nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            auto writer = POLL_UNWRAP_OR_RETURN(sender_.request_max_length_frame_writer());
            writer.build(PacketHeaderWriter{PacketType::DATA});
            return etl::move(writer);
        }

        nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            if (etl::holds_alternative<SendDataPacket>(state_)) {
                auto &state = etl::get<SendDataPacket>(state_);
                return state.send_frame(etl::move(reader));
            } else {
                return nb::pending;
            }
        }

      private:
        nb::Poll<void> execute_inner() {
            if (etl::holds_alternative<SendControlPacket<PacketType::SYN>>(state_)) {
                auto &state = etl::get<SendControlPacket<PacketType::SYN>>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(sender_, receiver_, time_));
                if (result.has_value()) {
                    state_ = SendDataPacket{TIMEOUT, MAX_RETRIES};
                } else {
                    return nb::ready();
                }
            }

            if (etl::holds_alternative<SendDataPacket>(state_)) {
                auto &state = etl::get<SendDataPacket>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(sender_, receiver_, time_));
                if (result.has_value()) {
                    state_ = SendControlPacket<PacketType::FIN>{TIMEOUT, MAX_RETRIES};
                } else {
                    return nb::ready();
                }
            }

            if (etl::holds_alternative<SendControlPacket<PacketType::FIN>>(state_)) {
                auto &state = etl::get<SendControlPacket<PacketType::FIN>>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(sender_, receiver_, time));
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
