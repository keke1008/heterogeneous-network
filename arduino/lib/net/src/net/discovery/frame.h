#pragma once

#include <etl/type_traits.h>
#include <net/frame.h>
#include <net/neighbor.h>
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

    class TotalCost {
        friend class ReceivedDiscoveryFrame;

        node::Cost cost;

        explicit TotalCost(node::Cost cost) : cost{cost} {}

        inline node::Cost get() const {
            return cost;
        }

      public:
        TotalCost(const TotalCost &) = default;
        TotalCost(TotalCost &&) = default;
        TotalCost &operator=(const TotalCost &) = default;
        TotalCost &operator=(TotalCost &&) = default;

        inline bool operator==(const TotalCost &rhs) const {
            return cost == rhs.cost;
        }

        inline bool operator!=(const TotalCost &rhs) const {
            return !(*this == rhs);
        }

        inline bool operator<(const TotalCost &rhs) const {
            return cost < rhs.cost;
        }

        inline bool operator<=(const TotalCost &rhs) const {
            return cost <= rhs.cost;
        }

        inline bool operator>(const TotalCost &rhs) const {
            return cost > rhs.cost;
        }

        inline bool operator>=(const TotalCost &rhs) const {
            return cost >= rhs.cost;
        }
    };

    struct DiscoveryFrame {
        DiscoveryFrameType type;
        frame::FrameId frame_id;
        node::Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        node::Source source;      // 探索を開始したノード
        node::Destination target; // 探索の対象となるノード
    };

    struct ReceivedDiscoveryFrame {
        DiscoveryFrameType type;
        frame::FrameId frame_id;
        node::Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        node::Source source;      // 探索を開始したノード
        node::Destination target; // 探索の対象となるノード
        node::NodeId previousHop; // このフレームを送信したノード

        node::Destination start_node() const {
            return type == DiscoveryFrameType::Request ? node::Destination(source) : target;
        }

        node::Destination destination() const {
            return type == DiscoveryFrameType::Request ? target : node::Destination(source);
        }

        TotalCost calculate_total_cost(node::Cost link_cost, node::Cost local_cost) const {
            return TotalCost{total_cost + link_cost + local_cost};
        }

        static DiscoveryFrame request(
            frame::FrameId frame_id,
            const node::Source &local,
            const node::Destination &target
        ) {
            return DiscoveryFrame{
                .type = DiscoveryFrameType::Request,
                .frame_id = frame_id,
                .total_cost = node::Cost(0),
                .source = local,
                .target = target,
            };
        }

        inline DiscoveryFrame repeat(const node::Source &local, TotalCost total_cost) const {
            return DiscoveryFrame{
                .type = type,
                .frame_id = frame_id,
                .total_cost = total_cost.get(),
                .source = source,
                .target = target,
            };
        }

        inline DiscoveryFrame
        reply(frame::FrameId frame_id, const node::Source &local, node::Cost total_cost) const {
            FASSERT(type == DiscoveryFrameType::Request);
            return DiscoveryFrame{
                .type = DiscoveryFrameType::Reply,
                .frame_id = frame_id,
                .total_cost = total_cost,
                .source = source,
                .target = target,
            };
        }

        inline DiscoveryFrame reply_by_cache(
            frame::FrameId frame_id,
            const node::Source &local,
            const TotalCost &total_cost
        ) const {
            FASSERT(type == DiscoveryFrameType::Request);
            return DiscoveryFrame{
                .type = DiscoveryFrameType::Reply,
                .frame_id = frame_id,
                .total_cost = total_cost.get(),
                .source = source,
                .target = target,
            };
        }
    };

    class AsyncDiscoveryFrameDeserializer {
        AsyncFrameTypeDeserializer type_;
        frame::AsyncFrameIdDeserializer frame_id_;
        node::AsyncCostDeserializer total_cost_;
        node::AsyncSourceDeserializer source_;
        node::AsyncDestinationDeserializer destination_;

      public:
        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(frame_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(total_cost_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(r));
            return destination_.deserialize(r);
        }

        inline void result() const {}

        inline ReceivedDiscoveryFrame received_result(const neighbor::ReceivedNeighborFrame &frame
        ) const {
            return ReceivedDiscoveryFrame{
                .type = type_.result(),
                .frame_id = frame_id_.result(),
                .total_cost = total_cost_.result(),
                .source = source_.result(),
                .target = destination_.result(),
                .previousHop = frame.sender,
            };
        }
    };

    class AsyncDiscoveryFrameSerializer {
        AsyncFrameTypeSerializer type_;
        frame::AsyncFrameIdSerializer frame_id_;
        node::AsyncCostSerializer total_cost_;
        node::AsyncSourceSerializer source_;
        node::AsyncDestinationSerializer destination_;

      public:
        explicit AsyncDiscoveryFrameSerializer(const DiscoveryFrame &frame)
            : type_{frame.type},
              frame_id_{frame.frame_id},
              total_cost_{frame.total_cost},
              source_{frame.source},
              destination_{frame.target} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(frame_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(total_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(w));
            return destination_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + frame_id_.serialized_length() +
                total_cost_.serialized_length() + source_.serialized_length() +
                destination_.serialized_length();
        }
    };
}; // namespace net::discovery
