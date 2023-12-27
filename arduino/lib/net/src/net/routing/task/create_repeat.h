#pragma once

#include "../frame.h"

namespace net::routing::task {
    class CreateRepeatFrameTask {
        node::Destination destination_;
        AsyncRoutingFrameHeaderSerializer header_serializer_;
        frame::AsyncFrameBufferReaderSerializer payload_serializer_;

      public:
        explicit CreateRepeatFrameTask(
            routing::RoutingFrame &&received_frame,
            const node::NodeId &sender_id
        )
            : destination_{received_frame.destination},
              header_serializer_{
                  received_frame.source,
                  received_frame.destination,
                  received_frame.frame_id,
              },
              payload_serializer_{etl::move(received_frame.payload)} {}

        const node::Destination &destination() const {
            return destination_;
        }

        template <nb::AsyncReadableWritable RW, uint8_t N>
        nb::Poll<frame::FrameBufferReader> execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborSocket<RW, N> &socket
        ) {
            if (!payload_serializer_.is_all_written()) {
                return nb::pending;
            }

            uint8_t length =
                header_serializer_.serialized_length() + payload_serializer_.serialized_length();
            frame::FrameBufferWriter &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_frame_writer(fs, lns, length));

            writer.serialize_all_at_once(header_serializer_);
            writer.serialize_all_at_once(payload_serializer_);
            return writer.create_reader();
        }
    };
} // namespace net::routing::task
