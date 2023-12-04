#pragma once

#include "./frame_id.h"
#include <net/neighbor.h>

namespace net::routing {
    struct UnicastRoutingFrameHeader {
        node::NodeId sender_id;
        node::NodeId target_id;
    };

    struct BroadcastRoutingFrameHeader {
        node::NodeId sender_id;
        FrameId frame_id;
    };

    using RoutingFrameHeader = etl::variant<UnicastRoutingFrameHeader, BroadcastRoutingFrameHeader>;

    class AsyncRoutingFrameHeaderDeserializer {
        node::AsyncNodeIdDeserializer sender_id_;
        node::AsyncNodeIdDeserializer target_id_;
        etl::optional<AsyncFrameIdDeserializer> frame_id_;

      public:
        inline RoutingFrameHeader result() const {
            if (frame_id_) {
                return BroadcastRoutingFrameHeader{sender_id_.result(), frame_id_->result()};
            } else {
                return UnicastRoutingFrameHeader{sender_id_.result(), target_id_.result()};
            }
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(sender_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(target_id_.deserialize(r));
            if (!target_id_.result().is_broadcast()) {
                return nb::DeserializeResult::Ok;
            }

            if (!frame_id_) {
                frame_id_.emplace();
            }
            return frame_id_->deserialize(r);
        }
    };

    class AsyncRoutingFrameHeaderSerializer {
        node::AsyncNodeIdSerializer sender_id_;
        node::AsyncNodeIdSerializer target_id_;
        nb::ser::Union<nb::ser::Empty, AsyncFrameIdSerializer> frame_id_;

      public:
        explicit AsyncRoutingFrameHeaderSerializer(const BroadcastRoutingFrameHeader &header)
            : sender_id_{header.sender_id},
              target_id_{node::NodeId::broadcast()},
              frame_id_{AsyncFrameIdSerializer{header.frame_id}} {}

        explicit AsyncRoutingFrameHeaderSerializer(const UnicastRoutingFrameHeader &header)
            : sender_id_{header.sender_id},
              target_id_{header.target_id},
              frame_id_{nb::ser::Empty{}} {}

        explicit AsyncRoutingFrameHeaderSerializer(const RoutingFrameHeader &header)
            : sender_id_{etl::visit([](const auto &header) { return header.sender_id; }, header)},
              target_id_{etl::visit(
                  util::Visitor{
                      [](const BroadcastRoutingFrameHeader &) { return node::NodeId::broadcast(); },
                      [](const UnicastRoutingFrameHeader &header) { return header.target_id; }
                  },
                  header
              )},
              frame_id_{etl::visit<etl::variant<nb::ser::Empty, FrameId>>(
                  util::Visitor{
                      [](const BroadcastRoutingFrameHeader &header) { return header.frame_id; },
                      [](const UnicastRoutingFrameHeader &) { return nb::ser::Empty{}; }
                  },
                  header
              )} {}

        inline uint8_t serialized_length() const {
            return sender_id_.serialized_length() + target_id_.serialized_length() +
                frame_id_.serialized_length();
        }

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(sender_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(target_id_.serialize(w));
            return frame_id_.serialize(w);
        }
    };

    struct RoutingFrame {
        RoutingFrameHeader header;
        frame::FrameBufferReader payload;

        const node::NodeId &sender_id() const {
            if (etl::holds_alternative<UnicastRoutingFrameHeader>(header)) {
                return etl::get<UnicastRoutingFrameHeader>(header).sender_id;
            } else {
                return etl::get<BroadcastRoutingFrameHeader>(header).sender_id;
            }
        }
    };

    struct UnicastRoutingFrame {
        UnicastRoutingFrameHeader header;
        frame::FrameBufferReader payload;

        explicit operator RoutingFrame() && {
            return RoutingFrame{header, etl::move(payload)};
        }
    };

    struct BroadcastRoutingFrame {
        BroadcastRoutingFrameHeader header;
        frame::FrameBufferReader payload;

        explicit operator RoutingFrame() && {
            return RoutingFrame{header, etl::move(payload)};
        }
    };
} // namespace net::routing
