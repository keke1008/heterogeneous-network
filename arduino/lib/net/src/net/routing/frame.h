#pragma once

#include <net/frame.h>
#include <net/neighbor.h>

namespace net::routing {
    struct RoutingFrameHeader {
        node::NodeId sender_id;
        node::NodeId target_id;
    };

    class AsyncRoutingFrameHeaderDeserializer {
        node::AsyncNodeIdDeserializer sender_id_;
        node::AsyncNodeIdDeserializer target_id_;

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
        node::AsyncNodeIdSerializer sender_id_;
        node::AsyncNodeIdSerializer target_id_;

      public:
        explicit AsyncRoutingFrameHeaderSerializer(const RoutingFrameHeader &header)
            : sender_id_{header.sender_id},
              target_id_{header.target_id} {}

        static constexpr inline uint8_t
        get_serialized_length(const node::NodeId &sender_id, const node::NodeId &target_id) {
            return node::AsyncNodeIdSerializer::get_serialized_length(sender_id) +
                node::AsyncNodeIdSerializer::get_serialized_length(target_id);
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
