#pragma once

#include "./service.h"
#include "./socket/task.h"
#include <net/link.h>

namespace net::neighbor {
    template <uint8_t DELAY_POOL_SIZE>
    class NeighborSocket {
        DelaySocket<ReceivedNeighborFrame, DELAY_POOL_SIZE> socket_;
        TaskExecutor task_executor_;

      public:
        explicit NeighborSocket(link::LinkSocket &&socket, NeighborSocketConfig config)
            : socket_{etl::move(socket), config} {}

        nb::Poll<ReceivedNeighborFrame> poll_receive_frame(util::Time &time) {
            return task_executor_.poll_receive_frame(socket_, time);
        }

        etl::expected<nb::Poll<void>, SendError> poll_send_frame(
            NeighborService &ns,
            const node::NodeId &remote_id,
            frame::FrameBufferReader &&reader
        ) {
            return task_executor_.poll_send_frame(ns, remote_id, etl::move(reader));
        }

        nb::Poll<void> poll_send_broadcast_frame(
            NeighborService &ns,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id = etl::nullopt
        ) {
            return task_executor_.poll_send_broadcast_frame(
                ns, socket_, etl::move(reader), ignore_id
            );
        }

        inline constexpr uint8_t max_payload_length() const {
            return socket_.max_payload_length();
        }

        nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            uint8_t payload_length
        ) {
            return (socket_.poll_frame_writer(fs, payload_length));
        }

        inline void execute(
            link::MediaService auto &ms,
            const local::LocalNodeService &lns,
            NeighborService &ns,
            util::Time &time
        ) {
            task_executor_.execute(ms, lns, ns, socket_, time);
        }
    };
} // namespace net::neighbor
