#pragma once

#include <net/discovery.h>
#include <net/neighbor.h>

namespace net::routing {
    using SendResult = etl::expected<void, neighbor::SendError>;

    struct RoutingFrameHeader {
        node::NodeId previous_hop;
        node::Source source;
        node::Destination destination;
        frame::FrameId frame_id;
    };

    struct RoutingFrame {
        node::NodeId previous_hop;
        node::Source source;
        node::Destination destination;
        frame::FrameId frame_id;
        frame::FrameBufferReader payload;

        RoutingFrame clone() const {
            return RoutingFrame{
                .previous_hop = previous_hop,
                .source = source,
                .destination = destination,
                .frame_id = frame_id,
                .payload = payload.make_initial_clone()
            };
        }
    };

    class AsyncRoutingFrameHeaderDeserializer {
        node::AsyncSourceDeserializer source_;
        node::AsyncDestinationDeserializer destination_;
        frame::AsyncFrameIdDeserializer frame_id_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(destination_.deserialize(reader));
            return frame_id_.deserialize(reader);
        }

        inline void result() const {}

        inline RoutingFrame as_frame(neighbor::ReceivedNeighborFrame &frame) const {
            return RoutingFrame{
                .previous_hop = frame.sender,
                .source = source_.result(),
                .destination = destination_.result(),
                .frame_id = frame_id_.result(),
                .payload = frame.reader.subreader()
            };
        }
    };

    class AsyncRoutingFrameHeaderSerializer {
        node::AsyncSourceSerializer source_;
        node::AsyncDestinationSerializer destination_;
        frame::AsyncFrameIdSerializer frame_id_;

      public:
        inline AsyncRoutingFrameHeaderSerializer(
            const node::Source &source,
            const node::Destination &destination,
            frame::FrameId frame_id
        )
            : source_{source},
              destination_{destination},
              frame_id_{frame_id} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(destination_.serialize(writer));
            return frame_id_.serialize(writer);
        }

        inline uint8_t serialized_length() const {
            return source_.serialized_length() + destination_.serialized_length() +
                frame_id_.serialized_length();
        }

        static uint8_t get_serialized_length(
            const node::Source &source,
            const node::Destination &destination,
            uint8_t payload_length
        ) {
            return node::AsyncSourceSerializer{source}.serialized_length() +
                node::AsyncDestinationSerializer{destination}.serialized_length() +
                frame::AsyncFrameIdSerializer::SERIALIZED_LENGTH + payload_length;
        }
    };

} // namespace net::routing
