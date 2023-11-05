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
        AsyncNodeIdParser sender_id_parser_;
        AsyncNodeIdParser target_id_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            POLL_UNWRAP_OR_RETURN(sender_id_parser_.parse(splitter));
            return target_id_parser_.parse(splitter);
        }

        inline RoutingFrameHeader result() {
            return RoutingFrameHeader{
                .sender_id = sender_id_parser_.result(),
                .target_id = target_id_parser_.result(),
            };
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
