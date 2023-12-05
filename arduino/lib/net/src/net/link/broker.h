#pragma once

#include "./media.h"
#include "net/frame/protocol_number.h"
#include <memory/lifetime.h>
#include <net/frame/service.h>
#include <tl/vec.h>

namespace net::link {
    class LinkFrameQueue {
        static constexpr uint8_t MAX_FRAME_BUFFER = 2;
        tl::Vec<LinkFrame, MAX_FRAME_BUFFER> received_frame_;
        tl::Vec<LinkFrame, MAX_FRAME_BUFFER> send_requested_frame_;

      public:
        LinkFrameQueue() = default;
        LinkFrameQueue(const LinkFrameQueue &) = delete;
        LinkFrameQueue(LinkFrameQueue &&) = delete;
        LinkFrameQueue &operator=(const LinkFrameQueue &) = delete;
        LinkFrameQueue &operator=(LinkFrameQueue &&) = delete;

        nb::Poll<void> poll_dispatch_received_frame(LinkFrame &&frame) {
            if (received_frame_.full()) {
                return nb::pending;
            } else {
                received_frame_.push_back(etl::move(frame));
                return nb::ready();
            }
        }

        nb::Poll<LinkFrame> poll_receive_frame(frame::ProtocolNumber protocol_number) {
            for (uint8_t i = 0; i < received_frame_.size(); i++) {
                if (received_frame_[i].protocol_number == protocol_number) {
                    return received_frame_.remove(i);
                }
            }
            return nb::pending;
        }

        nb::Poll<void> poll_request_send_frame(
            frame::ProtocolNumber protocol_number,
            const LinkAddress &remote,
            frame::FrameBufferReader &&reader
        ) {
            if (send_requested_frame_.full()) {
                return nb::pending;
            } else {
                send_requested_frame_.emplace_back(LinkFrame{
                    .protocol_number = protocol_number,
                    .remote = remote,
                    .reader = reader.origin(),
                });
                return nb::ready();
            }
        }

        nb::Poll<LinkFrame> poll_get_send_requested_frame(AddressType address_type) {
            for (uint8_t i = 0; i < send_requested_frame_.size(); i++) {
                auto &frame = send_requested_frame_[i];
                if (frame.remote.address_type() == address_type) {
                    return send_requested_frame_.remove(i);
                }
            }
            return nb::pending;
        }
    };

    class FrameBroker {
        memory::Static<LinkFrameQueue> &frame_queue_;

      public:
        FrameBroker() = delete;
        FrameBroker(const FrameBroker &) = default;
        FrameBroker(FrameBroker &&) = default;
        FrameBroker &operator=(const FrameBroker &) = delete;
        FrameBroker &operator=(FrameBroker &&) = delete;

        explicit FrameBroker(memory::Static<LinkFrameQueue> &frame_queue)
            : frame_queue_{frame_queue} {}

        inline nb::Poll<LinkFrame> poll_get_send_requested_frame(AddressType address_type) {
            return frame_queue_.get().poll_get_send_requested_frame(address_type);
        }

        inline nb::Poll<void> poll_dispatch_received_frame(LinkFrame &&frame) {
            return frame_queue_.get().poll_dispatch_received_frame(etl::move(frame));
        }
    };
} // namespace net::link
