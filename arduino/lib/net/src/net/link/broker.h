#pragma once

#include "./constants.h"
#include "./media.h"
#include <memory/lifetime.h>
#include <nb/time.h>
#include <net/frame.h>
#include <tl/vec.h>

namespace net::link {
    struct LinkReceivedFrame {
        LinkFrame frame;
        MediaPortNumber port;
        nb::Delay expiration;
    };

    struct LinkSendRequestedFrame {
        LinkFrame frame;
        etl::optional<MediaPortNumber> port;
        nb::Delay expiration;
    };

    class LinkFrameQueue {
        tl::Vec<LinkReceivedFrame, MAX_FRAME_BUFFER_SIZE> received_frame_;
        tl::Vec<LinkSendRequestedFrame, MAX_FRAME_BUFFER_SIZE> send_requested_frame_;
        nb::Debounce sweep_debounce_;

      public:
        LinkFrameQueue() = delete;
        LinkFrameQueue(const LinkFrameQueue &) = delete;
        LinkFrameQueue(LinkFrameQueue &&) = delete;
        LinkFrameQueue &operator=(const LinkFrameQueue &) = delete;
        LinkFrameQueue &operator=(LinkFrameQueue &&) = delete;

        explicit LinkFrameQueue(util::Time &time) : sweep_debounce_{time, FRAME_DROP_INTERVAL} {}

        void execute(util::Time &time) {
            if (sweep_debounce_.poll(time).is_pending()) {
                return;
            }

            for (uint8_t i = 0; i < received_frame_.size(); i++) {
                auto &frame = received_frame_[i];
                if (frame.expiration.poll(time).is_ready()) {
                    LOG_INFO(FLASH_STRING("Drop received frame: "), frame.frame.remote);
                    received_frame_.remove(i);
                }
            }

            for (uint8_t i = 0; i < send_requested_frame_.size(); i++) {
                auto &frame = send_requested_frame_[i];
                if (frame.expiration.poll(time).is_ready()) {
                    LOG_INFO(FLASH_STRING("Drop received frame: "), frame.frame.remote);
                    send_requested_frame_.remove(i);
                }
            }
        }

        nb::Poll<void>
        poll_dispatch_received_frame(LinkFrame &&frame, MediaPortNumber port, util::Time &time) {
            if (received_frame_.full()) {
                return nb::pending;
            } else {
                received_frame_.emplace_back(
                    etl::move(frame), port, nb::Delay{time, FRAME_EXPIRATION}
                );
                return nb::ready();
            }
        }

        nb::Poll<LinkReceivedFrame> poll_receive_frame(frame::ProtocolNumber protocol_number) {
            for (uint8_t i = 0; i < received_frame_.size(); i++) {
                if (received_frame_[i].frame.protocol_number == protocol_number) {
                    return received_frame_.remove(i);
                }
            }
            return nb::pending;
        }

        nb::Poll<void> poll_request_send_frame(
            frame::ProtocolNumber protocol_number,
            const Address &remote,
            frame::FrameBufferReader &&reader,
            etl::optional<MediaPortNumber> port,
            util::Time &time
        ) {
            FASSERT(reader.is_all_written());
            if (send_requested_frame_.full()) {
                return nb::pending;
            } else {
                send_requested_frame_.emplace_back(LinkSendRequestedFrame{
                    .frame =
                        LinkFrame{
                            .protocol_number = protocol_number,
                            .remote = remote,
                            .reader = reader.origin(),
                        },
                    .port = port,
                    .expiration = nb::Delay{time, FRAME_EXPIRATION},
                });
                return nb::ready();
            }
        }

        nb::Poll<LinkFrame> poll_get_send_requested_frame(
            AddressType address_type,
            MediaPortNumber port,
            const etl::optional<Address> &remote
        ) {
            for (uint8_t i = 0; i < send_requested_frame_.size(); i++) {
                auto &frame = send_requested_frame_[i];
                if (remote.has_value() && frame.frame.remote == *remote) {
                    return send_requested_frame_.remove(i).frame;
                }

                bool is_same_address_type = frame.frame.remote.type() == address_type;

                if (frame.port.has_value()) {
                    if (*frame.port != port) {
                        continue;
                    }
                    if (!is_same_address_type) {
                        send_requested_frame_.remove(i);
                        continue;
                    }
                    return send_requested_frame_.remove(i).frame;
                }

                if (!remote.has_value() && is_same_address_type) {
                    return send_requested_frame_.remove(i).frame;
                }
            }
            return nb::pending;
        }
    };

    class FrameBroker {
        memory::Static<LinkFrameQueue> &frame_queue_;
        MediaPortNumber port_;

      public:
        FrameBroker() = delete;
        FrameBroker(const FrameBroker &) = default;
        FrameBroker(FrameBroker &&) = default;
        FrameBroker &operator=(const FrameBroker &) = delete;
        FrameBroker &operator=(FrameBroker &&) = delete;

        explicit FrameBroker(memory::Static<LinkFrameQueue> &frame_queue, MediaPortNumber port)
            : frame_queue_{frame_queue},
              port_{port} {}

        inline nb::Poll<LinkFrame> poll_get_send_requested_frame(
            AddressType address_type,
            const etl::optional<Address> &remote = etl::nullopt
        ) {
            return frame_queue_.get().poll_get_send_requested_frame(address_type, port_, remote);
        }

        inline nb::Poll<void> poll_dispatch_received_frame(LinkFrame &&frame, util::Time &time) {
            return frame_queue_.get().poll_dispatch_received_frame(etl::move(frame), port_, time);
        }
    };
} // namespace net::link
