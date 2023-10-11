#pragma once

#include "../packet.h"
#include "./control.h"
#include "./data.h"
#include "./packet.h"

namespace net::trusted {
    template <frame::IFrameBufferRequester LowerRequester>
    class SenderSocket {
        static constexpr uint8_t MAX_RETRIES = 3;
        static constexpr util::Duration TIMEOUT = util::Duration::from_millis(1000);

        LowerRequester lower_requester_;

        using State = etl::variant<
            SendControlPacket<PacketType::SYN>,
            SendDataPacket,
            SendControlPacket<PacketType::FIN>>;

        State state_{SendControlPacket<PacketType::SYN>{TIMEOUT, MAX_RETRIES}};

        using Sender = etl::expected<DataPacketBufferSender, TrustedError>;
        using Error = etl::unexpected<TrustedError>;

        nb::Promise<Sender> sender_promise_;

      public:
        SenderSocket() = delete;
        SenderSocket(const SenderSocket &) = delete;
        SenderSocket(SenderSocket &&) = delete;
        SenderSocket &operator=(const SenderSocket &) = delete;
        SenderSocket &operator=(SenderSocket &&) = delete;

      public:
        explicit SenderSocket(LowerRequester &&lower_requester)
            : lower_requester_{etl::move(lower_requester)} {}

        template <
            frame::IFrameBufferRequester Requester,
            frame::IFrameSender Sender,
            frame::IFrameReceiver Receiver>
        nb::Poll<void>
        execute(Requester &requester, Sender &sender, Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<SendControlPacket<PacketType::SYN>>(state_)) {
                auto &state = etl::get<SendControlPacket<PacketType::SYN>>(state_);
                auto result =
                    POLL_UNWRAP_OR_RETURN(state.execute(requester, sender, receiver, time));
                if (!result.has_value()) {
                    sender_promise_.set_value(Sender{Error{result.error()}});
                    return nb::ready();
                }

                auto [tx, rx] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
                state_ = SendDataPacket{etl::move(rx), TIMEOUT, MAX_RETRIES};
                sender_promise_.set_value(Sender{DataPacketBufferSender{etl::move(tx)}});
            }

            if (etl::holds_alternative<SendDataPacket>(state_)) {
                auto &state = etl::get<SendDataPacket>(state_);
                auto result =
                    POLL_UNWRAP_OR_RETURN(state.execute(requester, sender, receiver, time));
                if (result.has_value()) {
                    state_ = SendControlPacket<PacketType::FIN>{TIMEOUT, MAX_RETRIES};
                } else {
                    return nb::ready();
                }
            }

            if (etl::holds_alternative<SendControlPacket<PacketType::FIN>>(state_)) {
                auto &state = etl::get<SendControlPacket<PacketType::FIN>>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(requester, sender, receiver, time));
                return nb::ready();
            }

            return nb::pending;
        }
    };
} // namespace net::trusted
