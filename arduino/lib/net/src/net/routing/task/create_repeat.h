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
        nb::Poll<etl::optional<frame::FrameBufferReader>> execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborSocket<RW, N> &socket
        ) {
            if (!payload_serializer_.is_all_written()) {
                return nb::pending;
            }

            uint8_t length =
                header_serializer_.serialized_length() + payload_serializer_.serialized_length();
            nb::Poll<frame::FrameBufferWriter> &&poll_writer =
                socket.poll_frame_writer(fs, lns, length);
            if (poll_writer.is_pending()) {
                // フレームバッファが取得できない場合は，バッファの余裕がないため，
                // 中継のために保持しているバッファを解放するために処理を終わらせる．
                LOG_INFO(FLASH_STRING("Frame buffer is not available discard frame"));
                return etl::optional<frame::FrameBufferReader>();
            }

            auto &&writer = poll_writer.unwrap();
            writer.serialize_all_at_once(header_serializer_);
            writer.serialize_all_at_once(payload_serializer_);
            return etl::optional(writer.create_reader());
        }
    };
} // namespace net::routing::task
