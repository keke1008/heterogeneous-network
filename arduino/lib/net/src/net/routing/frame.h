#pragma once

#include <net/discovery.h>
#include <net/neighbor.h>

namespace net::routing {
    struct RoutingFrame {
        node::NodeId source_id;
        discovery::Destination destination;
        frame::FrameId frame_id;
        frame::FrameBufferReader payload;
    };

    class AsyncRoutingFrameDeserializer {
        node::AsyncNodeIdDeserializer source_id_;
        discovery::AsyncDestinationDeserializer destination_;
        frame::AsyncFrameIdDeserializer frame_id_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(source_id_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(destination_.deserialize(reader));
            return frame_id_.deserialize(reader);
        }

        inline void result() const {}

        inline RoutingFrame as_frame(frame::FrameBufferReader &&reader) const {
            return RoutingFrame{
                .source_id = source_id_.result(),
                .destination = destination_.result(),
                .frame_id = frame_id_.result(),
                .payload = reader.subreader(),
            };
        }
    };

    class AsyncRoutingFrameSerializer {
        node::AsyncNodeIdSerializer source_id_;
        discovery::AsyncDestinationSerializer destination_;
        frame::AsyncFrameIdSerializer frame_id_;
        frame::AsyncFrameBufferReaderSerializer payload_;

      public:
        inline AsyncRoutingFrameSerializer(RoutingFrame &&frame)
            : source_id_{frame.source_id},
              destination_{frame.destination},
              frame_id_{frame.frame_id},
              payload_{etl::move(frame.payload)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &writer) {
            SERDE_SERIALIZE_OR_RETURN(source_id_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(destination_.serialize(writer));
            SERDE_SERIALIZE_OR_RETURN(frame_id_.serialize(writer));
            return payload_.serialize(writer);
        }

        inline uint8_t serialized_length() const {
            return source_id_.serialized_length() + destination_.serialized_length() +
                frame_id_.serialized_length() + payload_.serialized_length();
        }

        static uint8_t get_serialized_length(
            const node::NodeId &source_id,
            const discovery::Destination &destination,
            uint8_t payload_length
        ) {
            return node::AsyncNodeIdSerializer{source_id}.serialized_length() +
                discovery::AsyncDestinationSerializer{destination}.serialized_length() +
                frame::AsyncFrameIdSerializer::SERIALIZED_LENGTH + payload_length;
        }
    };

} // namespace net::routing
