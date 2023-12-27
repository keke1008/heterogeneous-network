#pragma once

#include <nb/serde.h>
#include <net/link.h>

namespace net::neighbor {
    template <nb::AsyncReadableWritable RW, typename Frame, uint8_t DELAY_POOL_SIZE>
    class CoreSocket {
        link::LinkSocket<RW> socket_;
        nb::DelayPool<Frame, DELAY_POOL_SIZE> delay_pool_{};

      public:
        explicit CoreSocket(link::LinkSocket<RW> &&socket) : socket_{etl::move(socket)} {}

        inline const link::LinkSocket<RW> &link_socket() const {
            return socket_;
        }

        inline uint8_t max_payload_length() const {
            return socket_.max_payload_length();
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_frame_writer(frame::FrameService &fs, uint8_t payload_length) {
            return socket_.poll_frame_writer(fs, payload_length);
        }

        inline nb::Poll<link::LinkReceivedFrame> poll_receive_link_frame() {
            return socket_.poll_receive_frame();
        }

        inline nb::Poll<void> poll_delaying_frame_pushable() {
            return delay_pool_.poll_pushable();
        }

        inline void
        poll_push_delaying_frame(Frame &&frame, util::Duration delay, util::Time &time) {
            delay_pool_.push(etl::move(frame), delay, time);
        }

        inline nb::Poll<Frame> poll_receive_frame(util::Time &time) {
            return delay_pool_.poll_pop_expired(time);
        }

        inline etl::expected<nb::Poll<void>, link::SendFrameError> poll_send_frame(
            const link::LinkAddress &remote,
            frame::FrameBufferReader &&reader,
            etl::optional<link::MediaPortNumber> port
        ) {
            return socket_.poll_send_frame(remote, etl::move(reader), port);
        }
    };
} // namespace net::neighbor