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

    enum class NodeNotificationEntryType : uint8_t {
        SelfUpdated = 1,
        NeighborUpdated = 2,
        NeighborRemoved = 3,
        FrameReceived = 4,
    };

    using AsyncNodeNotificationEntryTypeSerializer = nb::ser::Enum<NodeNotificationEntryType>;

    struct SelfUpdatedEntry {
        node::OptionalClusterId cluster_id;
        node::Cost cost;

        inline static SelfUpdatedEntry from_local(const notification::SelfUpdated &self_updated) {
            return SelfUpdatedEntry{
                .cluster_id = self_updated.cluster_id,
                .cost = self_updated.cost,
            };
        }
    };

    class AsyncSelfUpdateEntrySerializer {
        node::AsyncOptionalClusterIdSerializer cluster_id_;
        node::AsyncCostSerializer cost_;

      public:
        explicit AsyncSelfUpdateEntrySerializer(const SelfUpdatedEntry &self_updated)
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

    struct NeighborUpdatedEntry {
        node::NodeId neighbor;
        node::Cost link_cost;

        inline static NeighborUpdatedEntry
        from_local(const notification::NeighborUpdated &neighbor_updated) {
            return NeighborUpdatedEntry{
                .neighbor = neighbor_updated.neighbor_id, .link_cost = neighbor_updated.link_cost
            };
        }
    };

    class AsyncNeighborUpdatedEntrySerializer {
        node::AsyncNodeIdSerializer neighbor_;
        node::AsyncCostSerializer link_cost_;

      public:
        explicit AsyncNeighborUpdatedEntrySerializer(const NeighborUpdatedEntry &neighbor_updated)
            : neighbor_{etl::move(neighbor_updated.neighbor)},
              link_cost_{etl::move(neighbor_updated.link_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(neighbor_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(link_cost_.serialize(buffer));
            return nb::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return neighbor_.serialized_length() + link_cost_.serialized_length();
        }
    };

    struct NeighborRemovedEntry {
        node::NodeId neighbor_id;

        inline static NeighborRemovedEntry
        from_local(const notification::NeighborRemoved &neighbor_removed) {
            return NeighborRemovedEntry{neighbor_removed.neighbor_id};
        }
    };

    class AsyncNeighborRemovedEntrySerializer {
        node::AsyncNodeIdSerializer neighbor_id_;

      public:
        explicit AsyncNeighborRemovedEntrySerializer(const NeighborRemovedEntry &neighbor_removed)
            : neighbor_id_{etl::move(neighbor_removed.neighbor_id)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return neighbor_id_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return neighbor_id_.serialized_length();
        }
    };

    struct FrameReceivedEntry {};

    class AsyncFrameReceivedEntrySerializer {
      public:
        explicit AsyncFrameReceivedEntrySerializer(const FrameReceivedEntry &) {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return nb::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return 0;
        }
    };

    using NodeNotificationEntry = etl::
        variant<SelfUpdatedEntry, NeighborUpdatedEntry, NeighborRemovedEntry, FrameReceivedEntry>;

    class AsyncNodeNotificationEntrySerializer {
        nb::ser::Bin<uint8_t> frame_type_{static_cast<uint8_t>(FrameType::NodeNotification)};
        nb::ser::Variant<
            AsyncSelfUpdateEntrySerializer,
            AsyncNeighborUpdatedEntrySerializer,
            AsyncNeighborRemovedEntrySerializer,
            AsyncFrameReceivedEntrySerializer>
            notification_;

      public:
        explicit AsyncNodeNotificationEntrySerializer(const NodeNotificationEntry &notification)
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

    inline NodeNotificationEntry
    from_local_notification(const notification::LocalNotification &notification) {
        return etl::visit<NodeNotificationEntry>(
            util::Visitor{
                [](const notification::SelfUpdated &self_updated) {
                    return SelfUpdatedEntry::from_local(self_updated);
                },
                [](const notification::NeighborUpdated &neighbor_updated) {
                    return NeighborUpdatedEntry::from_local(neighbor_updated);
                },
                [](const notification::NeighborRemoved &neighbor_removed) {
                    return NeighborRemovedEntry::from_local(neighbor_removed);
                },
                [](const notification::FrameReceived &) { return FrameReceivedEntry{}; }
            },
            notification
        );
    }
} // namespace net::observer
