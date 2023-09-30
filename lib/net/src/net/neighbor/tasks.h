#pragma once

#include "./table.h"
#include <nb/poll.h>
#include <net/frame/service.h>
#include <net/link/address.h>
#include <net/protocol_number.h>

namespace net::neighbor {
    constexpr uint8_t NEIGHBOR_MESSAGE_SIZE = 1;

    enum class MessageType : uint8_t {
        Solicitation = 0x01,
        Advertisement = 0x02,
        Disconnect = 0x03,
    };

    struct NeighborFrame {
        MessageType message_type;
        link::Address destination;
    };

    class WaitForFrameReceiving {
      public:
        template <frame::IFrameService<link::Address> FrameService>
        nb::Poll<frame::FrameReceptionNotification<link::Address>>
        execute(FrameService &frame_service) {
            uint8_t protocol = static_cast<uint8_t>(protocol::ProtocolNumber::Neighbor);
            return frame_service.poll_reception_notification(protocol);
        }
    };

    class ReceiveFrame {
        frame::FrameReceptionNotification<link::Address> notification_;

      public:
        ReceiveFrame(frame::FrameReceptionNotification<link::Address> &&notification)
            : notification_{etl::move(notification)} {}

        nb::Poll<NeighborFrame> execute() {
            if (notification_.source.poll().is_pending() ||
                notification_.reader.readable_count() < 1) {
                return nb::pending;
            }

            return NeighborFrame{
                .message_type = static_cast<MessageType>(notification_.reader.read()),
                .destination = notification_.source.poll().unwrap().get(),
            };
        }
    };

    class SendFrame {
        NeighborFrame current_frame_;

      public:
        SendFrame(const NeighborFrame &frame) : current_frame_{frame} {}

        template <frame::IFrameService<link::Address> FrameService>
        nb::Poll<void> execute(FrameService &frame_service) {
            auto transmission = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_transmission(
                protocol::ProtocolNumber::Neighbor, current_frame_.destination,
                NEIGHBOR_MESSAGE_SIZE
            ));
            transmission.writer.write(static_cast<uint8_t>(current_frame_.message_type));
            return nb::ready();
        }
    };

    class Task {
        using State = etl::variant<WaitForFrameReceiving, ReceiveFrame, SendFrame>;
        State state_;

      public:
        Task() = default;
        Task(const Task &) = delete;
        Task(Task &&) = default;
        Task &operator=(const Task &) = delete;
        Task &operator=(Task &&) = default;

        template <frame::IFrameService<link::Address> FrameService>
        void execute(FrameService &frame_service, NeighborTable &neighbor_table) {
            while (true) {
                if (etl::holds_alternative<WaitForFrameReceiving>(state_)) {
                    auto poll_notification =
                        etl::get<WaitForFrameReceiving>(state_).execute(frame_service);
                    if (poll_notification.is_pending()) {
                        return;
                    }
                    state_ = ReceiveFrame{etl::move(poll_notification.unwrap())};
                }

                if (etl::holds_alternative<ReceiveFrame>(state_)) {
                    auto poll_frame = etl::get<ReceiveFrame>(state_).execute();
                    if (poll_frame.is_pending()) {
                        return;
                    }
                    auto frame = etl::move(poll_frame.unwrap());
                    if (frame.message_type == MessageType::Solicitation) {
                        if (neighbor_table.full()) {
                            state_ = SendFrame{NeighborFrame{
                                .message_type = MessageType::Disconnect,
                                .destination = frame.destination,
                            }};
                        } else {
                            neighbor_table.add_neighbor(Neighbor{.address = frame.destination});
                            state_ = SendFrame{NeighborFrame{
                                .message_type = MessageType::Solicitation,
                                .destination = frame.destination,
                            }};
                        }
                    } else if (frame.message_type == MessageType::Advertisement) {
                        neighbor_table.add_neighbor(Neighbor{.address = frame.destination});
                        state_ = WaitForFrameReceiving{};
                    } else if (frame.message_type == MessageType::Disconnect) {
                        neighbor_table.remove_neighbor(frame.destination);
                        state_ = WaitForFrameReceiving{};
                    }
                }

                if (etl::holds_alternative<SendFrame>(state_)) {
                    auto poll = etl::get<SendFrame>(state_).execute(frame_service);
                    if (poll.is_pending()) {
                        return;
                    }
                    state_ = WaitForFrameReceiving{};
                }
            }
        }
    };
} // namespace net::neighbor
