#pragma once

#include "./neighbor.h"
#include "./node.h"
#include <net/frame.h>

namespace net::routing {
    struct RoutingFrameHeader {
        NodeId sender_id;
        NodeId target_id;
    };

    class AsyncRoutingFrameHeaderDeserializer {
        AsyncNodeIdDeserializer sender_id_;
        AsyncNodeIdDeserializer target_id_;

      public:
        inline RoutingFrameHeader result() const {
            return RoutingFrameHeader{
                .sender_id = sender_id_.result(),
                .target_id = target_id_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(sender_id_.deserialize(r));
            return target_id_.deserialize(r);
        }
    };

    struct RoutingFrame {
        RoutingFrameHeader header;
        frame::FrameBufferReader payload;
    };

    inline uint8_t calculate_frame_header_length(const NodeId &sender_id, const NodeId &target_id) {
        return sender_id.serialized_length() + target_id.serialized_length();
    }

    inline void write_frame_header(
        frame::FrameBufferWriter &writer,
        const NodeId &sender_id,
        const NodeId &target_id
    ) {
        writer.write(sender_id, target_id);
    }
} // namespace net::routing
