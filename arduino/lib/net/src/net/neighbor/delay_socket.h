#pragma once

#include <nb/serde.h>
#include <net/link.h>

namespace net::neighbor {
    struct NeighborSocketConfig {
        bool do_delay;
    };

    template <typename Frame, uint8_t DELAY_POOL_SIZE>
    class DelaySocket {
        link::LinkSocket socket_;
        nb::DelayPool<Frame, DELAY_POOL_SIZE> delay_pool_{};
        NeighborSocketConfig config_;

      public:
        explicit DelaySocket(link::LinkSocket &&socket, NeighborSocketConfig config)
            : socket_{etl::move(socket)},
              config_{config} {}

        inline const link::LinkSocket &link_socket() const {
            return socket_;
        }

        inline uint8_t max_payload_length() const {
            return socket_.max_payload_length();
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_frame_writer(frame::FrameService &fs, uint8_t payload_length) {
            return socket_.poll_frame_writer(fs, payload_length);
        }

        inline nb::Poll<link::LinkFrame> poll_receive_link_frame() {
            return socket_.poll_receive_frame();
        }

        inline nb::Poll<void> poll_delaying_frame_pushable() {
            return delay_pool_.poll_pushable();
        }

        inline void push_delaying_frame(Frame &&frame, util::Duration delay, util::Time &time) {
            auto actual_delay = config_.do_delay ? delay : util::Duration::zero();
            delay_pool_.push(etl::move(frame), actual_delay, time);
        }

        inline nb::Poll<Frame> poll_receive_frame(util::Time &time) {
            return delay_pool_.poll_pop_expired(time);
        }

        inline etl::expected<nb::Poll<void>, link::SendFrameError> poll_send_frame(
            link::MediaPortMask media_port_mask,
            const link::Address &remote,
            frame::FrameBufferReader &&reader,
            util::Time &time
        ) {
            return socket_.poll_send_frame(media_port_mask, remote, etl::move(reader), time);
        }
    };
} // namespace net::neighbor
