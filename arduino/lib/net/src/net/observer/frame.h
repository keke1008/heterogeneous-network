#pragma once

#include <nb/serde.h>
#include <net/notification.h>
#include <stdint.h>

namespace net::observer {
    enum class FrameType : uint8_t {
        NodeSubscription = 1,
        NodeNotification = 2,
    };

    static constexpr bool is_valid_frame_type(uint8_t frame_type) {
        return frame_type == static_cast<uint8_t>(FrameType::NodeSubscription) ||
            frame_type == static_cast<uint8_t>(FrameType::NodeNotification);
    }

    using AsyncFrameTypeDeserializer = nb::de::Enum<FrameType, is_valid_frame_type>;

    enum class NodeNotificationType : uint8_t {
        SelfUpdated = 1,
        NeighborUpdated = 2,
        NeighborRemoved = 3,
        FrameReceived = 4,
    };

    using AsyncNodeNotificationTypeSerializer = nb::ser::Enum<NodeNotificationType>;

    struct SelfUpdated {
        node::OptionalClusterId cluster_id;
        node::Cost cost;

        inline static SelfUpdated from_local(const notification::SelfUpdated &self_updated) {
            return SelfUpdated{
                .cluster_id = self_updated.cluster_id,
                .cost = self_updated.cost,
            };
        }
    };

    class AsyncSelfUpdateSerializer {
        node::AsyncOptionalClusterIdSerializer cluster_id_;
        node::AsyncCostSerializer cost_;

      public:
        explicit AsyncSelfUpdateSerializer(const SelfUpdated &self_updated)
            : cluster_id_{etl::move(self_updated.cluster_id)},
              cost_{etl::move(self_updated.cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(cluster_id_.serialize(buffer));
            return cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return cluster_id_.serialized_length() + cost_.serialized_length();
        }
    };

    struct NeighborUpdated {
        node::OptionalClusterId local_cluster_id;
        node::Cost local_cost;
        node::NodeId neighbor;
        node::Cost link_cost;

        inline static NeighborUpdated from_local(
            const notification::NeighborUpdated &neighbor_updated,
            node::OptionalClusterId local_cluster_id,
            node::Cost local_cost
        ) {
            return NeighborUpdated{
                .local_cluster_id = local_cluster_id,
                .local_cost = local_cost,
                .neighbor = neighbor_updated.neighbor_id,
                .link_cost = neighbor_updated.link_cost
            };
        }
    };

    class AsyncNeighborUpdatedSerializer {
        node::AsyncOptionalClusterIdSerializer local_cluster_id_;
        node::AsyncCostSerializer local_cost_;
        node::AsyncNodeIdSerializer neighbor_;
        node::AsyncCostSerializer link_cost_;

      public:
        explicit AsyncNeighborUpdatedSerializer(const NeighborUpdated &neighbor_updated)
            : local_cluster_id_{etl::move(neighbor_updated.local_cluster_id)},
              local_cost_{etl::move(neighbor_updated.local_cost)},
              neighbor_{etl::move(neighbor_updated.neighbor)},
              link_cost_{etl::move(neighbor_updated.link_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(local_cluster_id_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(local_cost_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(neighbor_.serialize(buffer));
            return link_cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return local_cluster_id_.serialized_length() + local_cost_.serialized_length() +
                neighbor_.serialized_length() + link_cost_.serialized_length();
        }
    };

    struct NeighborRemoved {
        node::NodeId neighbor_id;

        inline static NeighborRemoved
        from_local(const notification::NeighborRemoved &neighbor_removed) {
            return NeighborRemoved{neighbor_removed.neighbor_id};
        }
    };

    class AsyncNeighborRemovedSerializer {
        node::AsyncNodeIdSerializer neighbor_id_;

      public:
        explicit AsyncNeighborRemovedSerializer(const NeighborRemoved &neighbor_removed)
            : neighbor_id_{etl::move(neighbor_removed.neighbor_id)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return neighbor_id_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return neighbor_id_.serialized_length();
        }
    };

    struct FrameReceived {};

    class AsyncFrameReceivedSerializer {
      public:
        explicit AsyncFrameReceivedSerializer(const FrameReceived &) {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return nb::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return 0;
        }
    };

    using NodeNotification =
        etl::variant<SelfUpdated, NeighborUpdated, NeighborRemoved, FrameReceived>;

    class AsyncNodeNotificationSerializer {
        nb::ser::Bin<uint8_t> frame_type_{static_cast<uint8_t>(FrameType::NodeNotification)};
        nb::ser::Variant<
            AsyncSelfUpdateSerializer,
            AsyncNeighborUpdatedSerializer,
            AsyncNeighborRemovedSerializer,
            AsyncFrameReceivedSerializer>
            notification_;

      public:
        explicit AsyncNodeNotificationSerializer(const NodeNotification &notification)
            : notification_{notification} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(buffer));
            return notification_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return frame_type_.serialized_length() + notification_.serialized_length();
        }
    };

    struct NodeSubscriptionFrame {};

    class AsyncNodeSubscriptionFrameDeserializer {

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &buffer) {
            return nb::DeserializeResult::Ok;
        }

        inline NodeSubscriptionFrame result() const {
            return NodeSubscriptionFrame{};
        }
    };

    inline NodeNotification from_local_notification(
        const notification::LocalNotification &notification,
        node::OptionalClusterId local_cluster_id,
        node::Cost local_cost
    ) {
        return etl::visit<NodeNotification>(
            util::Visitor{
                [](const notification::SelfUpdated &self_updated) {
                    return SelfUpdated::from_local(self_updated);
                },
                [local_cost,
                 local_cluster_id](const notification::NeighborUpdated &neighbor_updated) {
                    return NeighborUpdated::from_local(
                        neighbor_updated, local_cluster_id, local_cost
                    );
                },
                [](const notification::NeighborRemoved &neighbor_removed) {
                    return NeighborRemoved::from_local(neighbor_removed);
                },
                [](const notification::FrameReceived &) { return FrameReceived{}; }
            },
            notification
        );
    }
} // namespace net::observer
