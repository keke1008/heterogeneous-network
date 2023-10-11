#pragma once

#include "../packet.h"
#include "./control.h"
#include "./data.h"
#include <net/link.h>

namespace net::trusted {
    template <frame::IFrameBufferRequester LowerRequester>
    class DataPacketBufferRequester {
        LowerRequester requester_;

        inline void write_header(frame::FrameBufferWriter &&writer) {
            writer.build(PacketHeaderWriter{PacketType::DATA});
        }

      public:
        explicit DataPacketBufferRequester(frame::FrameService<link::Address> &frame_service)
            : requester_{frame_service} {}

        inline nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(requester_.request_frame_writer(length));
            write_header(etl::move(writer));
            return etl::move(writer);
        }

        inline nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(requester_.request_max_length_frame_writer());
            write_header(etl::move(writer));
            return etl::move(writer);
        }
    };

    class SenderSocket {
        static constexpr uint8_t MAX_RETRIES = 3;
        static constexpr util::Duration TIMEOUT = util::Duration::from_millis(1000);

        using State = etl::variant<
            SendControlPacket<PacketType::SYN>,
            SendDataPacket,
            SendControlPacket<PacketType::FIN>>;

        State state_{SendControlPacket<PacketType::SYN>{TIMEOUT, MAX_RETRIES}};

        using Error = etl::unexpected<TrustedError>;

      public:
        SenderSocket() = default;
        SenderSocket(const SenderSocket &) = delete;
        SenderSocket(SenderSocket &&) = delete;
        SenderSocket &operator=(const SenderSocket &) = delete;
        SenderSocket &operator=(SenderSocket &&) = delete;

      public:
        template <frame::IFrameBufferRequester LowerRequester>
        inline DataPacketBufferRequester<LowerRequester>
        get_buffer_requester(LowerRequester &&lower_requester) {
            return DataPacketBufferRequester{etl::move(lower_requester)};
        }

        nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            if (etl::holds_alternative<SendDataPacket>(state_)) {
                auto &state = etl::get<SendDataPacket>(state_);
                return state.send_frame(etl::move(reader));
            } else {
                return nb::pending;
            }
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
                if (result.has_value()) {
                    state_ = SendDataPacket{TIMEOUT, MAX_RETRIES};
                } else {
                    return nb::ready();
                }
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

    static_assert(frame::IFrameSender<SenderSocket>);
} // namespace net::trusted
