#pragma once

#include "./service.h"
#include "./socket/task.h"
#include <net/link.h>

namespace net::neighbor {
    template <nb::AsyncReadableWritable RW, uint8_t DELAY_POOL_SIZE>
    class NeighborSocket {
        CoreSocket<RW, NeighborFrame, DELAY_POOL_SIZE> socket_;
        TaskExecutor task_executor_;

      public:
        explicit NeighborSocket(link::LinkSocket<RW> &&socket) : socket_{etl::move(socket)} {}

        nb::Poll<NeighborFrame> poll_receive_frame(util::Time &time) {
            return task_executor_.poll_receive_frame(socket_, time);
        }

        etl::expected<nb::Poll<void>, SendError> poll_send_frame(
            NeighborService<RW> &ns,
            const node::NodeId &remote_id,
            frame::FrameBufferReader &&reader
        ) {
            return task_executor_.poll_send_frame(ns, remote_id, etl::move(reader));
        }

        nb::Poll<void> poll_send_broadcast_frame(
            NeighborService<RW> &ns,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id = etl::nullopt
        ) {
            return task_executor_.poll_send_broadcast_frame(
                ns, socket_, etl::move(reader), ignore_id
            );
        }

        inline nb::Poll<uint8_t> max_payload_length(const local::LocalNodeService &lns) const {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            AsyncNeighborFrameHeaderSerializer serializer{info.source};
            return socket_.max_payload_length() - serializer.serialized_length();
        }

        nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            uint8_t payload_length
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            FASSERT(payload_length <= POLL_UNWRAP_OR_RETURN(max_payload_length(lns)));

            AsyncNeighborFrameHeaderSerializer serializer{info.source};
            uint8_t length = serializer.serialized_length() + payload_length;
            frame::FrameBufferWriter &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(socket_.poll_frame_writer(fs, length));
            writer.serialize_all_at_once(serializer);
            return writer.subwriter();
        }

        inline void
        execute(const local::LocalNodeService &lns, NeighborService<RW> &ns, util::Time &time) {
            const auto &poll_info = lns.poll_info();
            if (poll_info.is_ready()) {
                task_executor_.execute(ns, socket_, poll_info.unwrap(), time);
            }
        }
    };
} // namespace net::neighbor
