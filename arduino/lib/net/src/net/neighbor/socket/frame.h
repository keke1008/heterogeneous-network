#pragma once

#include <net/node.h>

namespace net::neighbor {
    struct NeighborFrame {
        node::Source sender;
        frame::FrameBufferReader reader;
    };

    struct NeighborFrameHeader {
        node::Source sender;

        inline NeighborFrame to_frame(frame::FrameBufferReader &&reader) const {
            return NeighborFrame{.sender = sender, .reader = reader.subreader()};
        }
    };

    class AsyncNeighborFrameHeaderDeserializer {
        node::AsyncSourceDeserializer sender_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &reader) {
            return sender_.deserialize(reader);
        }

        inline NeighborFrameHeader result() {
            return NeighborFrameHeader{.sender = sender_.result()};
        }
    };

    class AsyncNeighborFrameHeaderSerializer {
        node::AsyncSourceSerializer sender_;

      public:
        explicit AsyncNeighborFrameHeaderSerializer(const node::Source &sender) : sender_{sender} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &writer) {
            return sender_.serialize(writer);
        }

        inline uint8_t serialized_length() const {
            return sender_.serialized_length();
        }

        static inline uint8_t max_serialized_length() {
            return node::AsyncSourceSerializer::max_serialized_length();
        }
    };
} // namespace net::neighbor
