#pragma once

#include "./constants.h"
#include "./media.h"
#include <memory/lifetime.h>
#include <nb/time.h>
#include <net/frame.h>
#include <tl/vec.h>

namespace net::link {
    struct Entry {
        LinkFrame frame;
        nb::Delay expiration;
    };

    class LinkFrameQueue {
        tl::Vec<Entry, MAX_FRAME_BUFFER_SIZE> received_frame_;
        tl::Vec<Entry, MAX_FRAME_BUFFER_SIZE> send_requested_frame_;
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
                auto &entry = received_frame_[i];
                if (entry.expiration.poll(time).is_ready()) {
                    LOG_INFO(FLASH_STRING("Drop recv frame: "), entry.frame.remote);
                    received_frame_.remove(i);
                }
            }

            for (uint8_t i = 0; i < send_requested_frame_.size(); i++) {
                auto &entry = send_requested_frame_[i];
                if (entry.expiration.poll(time).is_ready()) {
                    LOG_INFO(FLASH_STRING("Drop send req frame: "), entry.frame.remote);
                    send_requested_frame_.remove(i);
                }
            }
        }

        nb::Poll<void> poll_dispatch_received_frame(
            link::MediaPortNumber media_port,
            frame::ProtocolNumber protocol_number,
            const Address &remote,
            frame::FrameBufferReader &&reader,
            util::Time &time
        ) {
            if (received_frame_.full()) {
                return nb::pending;
            } else {
                received_frame_.emplace_back(
                    LinkFrame{
                        .media_port_mask = MediaPortMask::from_port_number(media_port),
                        .protocol_number = protocol_number,
                        .remote = remote,
                        .reader = reader.origin(),
                    },
                    nb::Delay{time, FRAME_EXPIRATION}
                );
                return nb::ready();
            }
        }

        nb::Poll<LinkFrame> poll_receive_frame(frame::ProtocolNumber protocol_number) {
            for (uint8_t i = 0; i < received_frame_.size(); i++) {
                if (received_frame_[i].frame.protocol_number == protocol_number) {
                    return received_frame_.remove(i).frame;
                }
            }
            return nb::pending;
        }

        nb::Poll<void> poll_request_send_frame(
            MediaPortMask media_port_mask,
            frame::ProtocolNumber protocol_number,
            const Address &remote,
            frame::FrameBufferReader &&reader,
            util::Time &time
        ) {
            if (send_requested_frame_.full()) {
                return nb::pending;
            }

            send_requested_frame_.emplace_back(
                LinkFrame{
                    .media_port_mask = media_port_mask,
                    .protocol_number = protocol_number,
                    .remote = remote,
                    .reader = reader.origin(),
                },
                nb::Delay{time, FRAME_EXPIRATION}
            );
            return nb::ready();
        }

        nb::Poll<LinkFrame>
        poll_get_send_requested_frame(MediaPortNumber port, link::AddressType address_type) {
            for (uint8_t i = 0; i < send_requested_frame_.size(); i++) {
                auto &entry = send_requested_frame_[i];

                if (entry.frame.media_port_mask.is_unspecified()) {
                    if (entry.frame.remote.type() == address_type) {
                        return send_requested_frame_.remove(i).frame;
                    } else {
                        continue;
                    }
                }

                if (entry.frame.media_port_mask.test(port)) {
                    return send_requested_frame_.remove(i).frame;
                }
            }
            return nb::pending;
        }
    };

    class FrameBroker {
        memory::Static<LinkFrameQueue> &frame_queue_;
        etl::optional<MediaPortNumber> port_;

      public:
        FrameBroker() = delete;
        FrameBroker(const FrameBroker &) = delete;
        FrameBroker(FrameBroker &&) = default;
        FrameBroker &operator=(const FrameBroker &) = delete;
        FrameBroker &operator=(FrameBroker &&) = delete;

        explicit FrameBroker(memory::Static<LinkFrameQueue> &frame_queue)
            : frame_queue_{frame_queue} {}

        inline void initialize_media_port(MediaPortNumber port) {
            FASSERT(!port_.has_value());
            port_ = port;
        }

        inline nb::Poll<LinkFrame> poll_get_send_requested_frame(AddressType address_type) {
            FASSERT(port_.has_value());
            return frame_queue_.get().poll_get_send_requested_frame(*port_, address_type);
        }

        inline nb::Poll<void> poll_dispatch_received_frame(
            frame::ProtocolNumber protocol_number,
            const Address &remote,
            frame::FrameBufferReader &&reader,
            util::Time &time
        ) {
            FASSERT(port_.has_value());
            return frame_queue_.get().poll_dispatch_received_frame(
                *port_, protocol_number, remote, reader.origin(), time
            );
        }
    };
} // namespace net::link
