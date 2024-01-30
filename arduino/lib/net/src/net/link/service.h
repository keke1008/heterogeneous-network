#pragma once

#include "./broker.h"
#include "./media.h"
#include <etl/bitset.h>
#include <etl/expected.h>
#include <nb/poll.h>

namespace net::link {
    class ProtocolLock {
        etl::bitset<frame::NUM_PROTOCOLS> locked_protocol_;

      public:
        inline bool is_locked(frame::ProtocolNumber protocol_number) {
            uint8_t i = static_cast<uint8_t>(protocol_number);
            return locked_protocol_.test(i);
        }

        inline void lock(frame::ProtocolNumber protocol_number) {
            uint8_t i = static_cast<uint8_t>(protocol_number);
            locked_protocol_.set(i);
        }
    };

    enum class SendFrameError {
        SupportedMediaNotFound,
        BroadcastNotSupported,
    };

    class LinkSocket {
        memory::Static<MeasuredLinkFrameQueue> &queue_;
        frame::ProtocolNumber protocol_number_;

      public:
        LinkSocket() = delete;
        LinkSocket(const LinkSocket &) = delete;
        LinkSocket(LinkSocket &&) = default;
        LinkSocket &operator=(const LinkSocket &) = delete;
        LinkSocket &operator=(LinkSocket &&) = delete;

        explicit LinkSocket(
            memory::Static<MeasuredLinkFrameQueue> &queue,
            frame::ProtocolNumber protocol_number
        )
            : queue_{queue},
              protocol_number_{protocol_number} {}

        inline nb::Poll<LinkFrame> poll_receive_frame(util::Time &time) {
            return queue_.get().poll_receive_frame(protocol_number_, time);
        }

        inline etl::expected<nb::Poll<void>, SendFrameError> poll_send_frame(
            MediaPortMask media_port_mask,
            const Address &remote,
            frame::FrameBufferReader &&reader,
            util::Time &time
        ) {
            return queue_.get().poll_request_send_frame(
                media_port_mask, protocol_number_, remote, etl::move(reader), time
            );
        }

        inline constexpr uint8_t max_payload_length() const {
            return frame::MTU;
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_frame_writer(frame::FrameService &frame_service, uint8_t payload_length) {
            return frame_service.request_frame_writer(payload_length);
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_max_length_frame_writer(frame::FrameService &frame_service) {
            return frame_service.request_max_length_frame_writer();
        }
    };

    class LinkService {
        ProtocolLock lock_;
        memory::Static<MeasuredLinkFrameQueue> &queue_;

      public:
        LinkService() = delete;
        LinkService(const LinkService &) = delete;
        LinkService(LinkService &&) = delete;
        LinkService &operator=(const LinkService &) = delete;
        LinkService &operator=(LinkService &&) = delete;

        explicit LinkService(memory::Static<MeasuredLinkFrameQueue> &queue) : queue_{queue} {}

        inline LinkSocket open(frame::ProtocolNumber protocol_number) {
            FASSERT(!lock_.is_locked(protocol_number));
            lock_.lock(protocol_number);
            return LinkSocket{queue_, protocol_number};
        }

        inline Measurement &measurement() {
            return queue_.get().measurement();
        }
    };
}; // namespace net::link
