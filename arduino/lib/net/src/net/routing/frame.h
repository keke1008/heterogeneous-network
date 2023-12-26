#pragma once

#include <net/discovery.h>
#include <net/neighbor.h>

namespace net::routing {
    using SendResult = etl::expected<void, neighbor::SendError>;

    struct RoutingFrameHeader {
        node::Source source;
        node::Destination destination;
        node::NodeId previous_hop;
        frame::FrameId frame_id;
    };

    struct RoutingFrame {
        node::Source source;
        node::Destination destination;
        node::NodeId previous_hop;
        frame::FrameId frame_id;
        frame::FrameBufferReader payload;

        RoutingFrame clone() const {
            return RoutingFrame{
                .source = source,
                .destination = destination,
                .previous_hop = previous_hop,
                .frame_id = frame_id,
                .payload = payload.make_initial_clone()
            };
        }
    };

    class AsyncRoutingFrameHeaderDeserializer {
        node::AsyncSourceDeserializer source_;
        node::AsyncDestinationDeserializer destination_;
        node::AsyncNodeIdDeserializer previous_hop_;
        frame::AsyncFrameIdDeserializer frame_id_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(destination_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(previous_hop_.deserialize(reader));
            return frame_id_.deserialize(reader);
        }

        inline void result() const {}

        inline RoutingFrame as_frame(frame::FrameBufferReader &&reader) const {
            return RoutingFrame{
                .source = source_.result(),
                .destination = destination_.result(),
                .previous_hop = previous_hop_.result(),
                .frame_id = frame_id_.result(),
                .payload = reader.subreader(),
            };
        }
    };

    class AsyncRoutingFrameHeaderSerializer {
        node::AsyncSourceSerializer source_;
        node::AsyncDestinationSerializer destination_;
        node::AsyncNodeIdSerializer previous_hop_;
        frame::AsyncFrameIdSerializer frame_id_;

      public:
        inline AsyncRoutingFrameHeaderSerializer(
            const node::Source &source,
            const node::Destination &destination,
            const node::NodeId &previous_hop,
            frame::FrameId frame_id
        )
            : source_{source},
              destination_{destination},
              previous_hop_{previous_hop},
              frame_id_{frame_id} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(destination_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(previous_hop_.serialize(writer));
            return frame_id_.serialize(writer);
        }

        inline uint8_t serialized_length() const {
            return source_.serialized_length() + destination_.serialized_length() +
                previous_hop_.serialized_length() + frame_id_.serialized_length();
        }

        static uint8_t get_serialized_length(
            const node::Source &source,
            const node::Destination &destination,
            const node::NodeId &previous_hop,
            uint8_t payload_length
        ) {
            return node::AsyncSourceSerializer{source}.serialized_length() +
                node::AsyncDestinationSerializer{destination}.serialized_length() +
                node::AsyncNodeIdSerializer{previous_hop}.serialized_length() +
                frame::AsyncFrameIdSerializer::SERIALIZED_LENGTH + payload_length;
        }
    };

} // namespace net::routing
