#pragma once

#include "./destination.h"
#include <etl/type_traits.h>
#include <net/frame.h>
#include <net/node.h>
#include <stdint.h>

namespace net::discovery {
    enum class DiscoveryFrameType : uint8_t {
        Request = 0x01,
        Reply = 0x02,
    };

    inline bool is_valid_frame_type(uint8_t type) {
        return type <= static_cast<uint8_t>(DiscoveryFrameType::Reply);
    }

    using AsyncFrameTypeSerializer = nb::ser::Enum<DiscoveryFrameType>;
    using AsyncFrameTypeDeserializer = nb::de::Enum<DiscoveryFrameType, is_valid_frame_type>;

    struct DiscoveryFrame {
        DiscoveryFrameType type;
        frame::FrameId frame_id;
        node::Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        node::NodeId source_id; // 探索を開始したノード
        Destination destination;
        node::NodeId sender_id; // このフレームを送信したノード

        static DiscoveryFrame request(
            frame::FrameId frame_id,
            const node::NodeId &local_id,
            node::Cost self_cost,
            const Destination &destination
        ) {
            return DiscoveryFrame{
                .type = DiscoveryFrameType::Request,
                .frame_id = frame_id,
                .total_cost = self_cost,
                .source_id = local_id,
                .destination = destination,
                .sender_id = local_id,
            };
        }

        inline DiscoveryFrame repeat(node::Cost additional_cost) const {
            return DiscoveryFrame{
                .type = type,
                .frame_id = frame_id,
                .total_cost = total_cost + additional_cost,
                .source_id = source_id,
                .destination = destination,
                .sender_id = sender_id,
            };
        }

        template <nb::AsyncReadableWritable RW>
        inline DiscoveryFrame
        reply(link::LinkService<RW> &ls, const node::NodeId &self_id, frame::FrameId frame_id) {
            ASSERT(type == DiscoveryFrameType::Request);
            return DiscoveryFrame{
                .type = DiscoveryFrameType::Reply,
                .frame_id = frame_id,
                .total_cost = node::Cost(0),
                .source_id = source_id,
                .destination = destination,
                .sender_id = self_id,
            };
        }
    };

    class AsyncDiscoveryFrameDeserializer {
        AsyncFrameTypeDeserializer type_;
        frame::AsyncFrameIdDeserializer frame_id_;
        node::AsyncCostDeserializer total_cost_;
        node::AsyncNodeIdDeserializer source_id_;
        AsyncDestinationDeserializer destination_;
        node::AsyncNodeIdDeserializer sender_id_;

      public:
        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(frame_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(total_cost_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(destination_.deserialize(r));
            return sender_id_.deserialize(r);
        }

        inline DiscoveryFrame result() const {
            return DiscoveryFrame{
                .type = type_.result(),
                .frame_id = frame_id_.result(),
                .total_cost = total_cost_.result(),
                .source_id = source_id_.result(),
                .destination = destination_.result(),
                .sender_id = sender_id_.result(),
            };
        }
    };

    class AsyncDiscoveryFrameSerializer {
        AsyncFrameTypeSerializer type_;
        frame::AsyncFrameIdSerializer frame_id_;
        node::AsyncCostSerializer total_cost_;
        node::AsyncNodeIdSerializer source_id_;
        AsyncDestinationSerializer destination_;
        node::AsyncNodeIdSerializer sender_id_;

      public:
        explicit AsyncDiscoveryFrameSerializer(const DiscoveryFrame &frame)
            : type_{frame.type},
              frame_id_{frame.frame_id},
              total_cost_{frame.total_cost},
              source_id_{frame.source_id},
              destination_{frame.destination},
              sender_id_{frame.sender_id} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(frame_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(total_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(destination_.serialize(w));
            return sender_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + frame_id_.serialized_length() +
                total_cost_.serialized_length() + source_id_.serialized_length() +
                destination_.serialized_length() + sender_id_.serialized_length();
        }
    };
}; // namespace net::discovery
