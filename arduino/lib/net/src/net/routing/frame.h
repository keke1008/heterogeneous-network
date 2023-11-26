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

    class AsyncRoutingFrameHeaderSerializer {
        AsyncNodeIdSerializer sender_id_;
        AsyncNodeIdSerializer target_id_;

      public:
        explicit AsyncRoutingFrameHeaderSerializer(const RoutingFrameHeader &header)
            : sender_id_{header.sender_id},
              target_id_{header.target_id} {}

        static constexpr inline uint8_t
        get_serialized_length(const NodeId &sender_id, const NodeId &target_id) {
            return AsyncNodeIdSerializer::get_serialized_length(sender_id) +
                AsyncNodeIdSerializer::get_serialized_length(target_id);
        }

        inline uint8_t serialized_length() const {
            return sender_id_.serialized_length() + target_id_.serialized_length();
        }

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(sender_id_.serialize(w));
            return target_id_.serialize(w);
        }
    };

    struct RoutingFrame {
        RoutingFrameHeader header;
        frame::FrameBufferReader payload;
    };
} // namespace net::routing
