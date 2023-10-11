#pragma once

#include "../packet.h"
#include "./control.h"
#include "./data.h"
#include "./packet.h"

namespace net::trusted {
    class SenderSocket {
        static constexpr uint8_t MAX_RETRIES = 3;
        static constexpr util::Duration TIMEOUT = util::Duration::from_millis(1000);

        using State = etl::variant<
            SendControlPacket<PacketType::SYN>,
            SendDataPacket,
            SendControlPacket<PacketType::FIN>>;

        State state_{SendControlPacket<PacketType::SYN>{TIMEOUT, MAX_RETRIES}};
        nb::OneBufferReceiver<frame::FrameBufferReader> reader_rx_;

        using Error = etl::unexpected<TrustedError>;

      public:
        SenderSocket() = delete;
        SenderSocket(const SenderSocket &) = delete;
        SenderSocket(SenderSocket &&) = delete;
        SenderSocket &operator=(const SenderSocket &) = delete;
        SenderSocket &operator=(SenderSocket &&) = delete;

      public:
        explicit SenderSocket(nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx)
            : reader_rx_{etl::move(reader_rx)} {}

        template <frame::IFrameBufferRequester LowerRequester>
        inline DataPacketBufferRequester<LowerRequester>
        get_buffer_requester(LowerRequester &&lower_requester) {
            return DataPacketBufferRequester{etl::move(lower_requester)};
        }

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
                    etl::move(reader_rx_).close();
                    return nb::ready();
                }

                state_ = SendDataPacket{etl::move(reader_rx_), TIMEOUT, MAX_RETRIES};
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

    inline etl::pair<SenderSocket, DataPacketBufferSender> make_sender_socket() {
        auto [tx, rx] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return etl::make_pair(SenderSocket{etl::move(rx)}, DataPacketBufferSender{etl::move(tx)});
    }
} // namespace net::trusted
