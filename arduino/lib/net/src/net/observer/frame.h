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

    class AsyncFrameTypeDeserializer {
        nb::de::Bin<uint8_t> frame_type_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &buffer) {
            POLL_UNWRAP_OR_RETURN(frame_type_.deserialize(buffer));
            return is_valid_frame_type(frame_type_.result()) ? nb::DeserializeResult::Ok
                                                             : nb::DeserializeResult::Invalid;
        }

        inline FrameType result() const {
            ASSERT(is_valid_frame_type(frame_type_.result()));
            return static_cast<FrameType>(frame_type_.result());
        }
    };

    enum class NodeNotificationType : uint8_t {
        SelfUpdated = 1,
        NeighborUpdated = 2,
        NeighborRemoved = 3,
    };

    class AsyncNodeNotificationTypeSerializer {
        nb::ser::Bin<uint8_t> serializer_;

      public:
        explicit AsyncNodeNotificationTypeSerializer(NodeNotificationType notification_type)
            : serializer_{static_cast<uint8_t>(notification_type)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return serializer_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return serializer_.serialized_length();
        }
    };

    struct SelfUpdated {
        node::Cost cost;

        inline static SelfUpdated from_local(const notification::SelfUpdated &self_updated) {
            return SelfUpdated{self_updated.cost};
        }
    };

    class AsyncSelfUpdateSerializer {
        AsyncNodeNotificationTypeSerializer notification_type_{NodeNotificationType::SelfUpdated};
        node::AsyncCostSerializer cost_;

      public:
        explicit AsyncSelfUpdateSerializer(const SelfUpdated &self_updated)
            : cost_{etl::move(self_updated.cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + cost_.serialized_length();
        }
    };

    struct NeighborUpdated {
        node::Cost local_cost;
        node::NodeId neighbor_id;
        node::Cost neighbor_cost;
        node::Cost link_cost;

        inline static NeighborUpdated
        from_local(const notification::NeighborUpdated &neighbor_updated, node::Cost local_cost) {
            return NeighborUpdated{
                .local_cost = local_cost,
                .neighbor_id = neighbor_updated.neighbor_id,
                .neighbor_cost = neighbor_updated.neighbor_cost,
                .link_cost = neighbor_updated.link_cost
            };
        }
    };

    class AsyncNeighborUpdatedSerializer {

        AsyncNodeNotificationTypeSerializer notification_type_{NodeNotificationType::NeighborUpdated
        };
        node::AsyncCostSerializer local_cost_;
        node::AsyncNodeIdSerializer neighbor_id_;
        node::AsyncCostSerializer neighbor_cost_;
        node::AsyncCostSerializer link_cost_;

      public:
        explicit AsyncNeighborUpdatedSerializer(const NeighborUpdated &neighbor_updated)
            : local_cost_{etl::move(neighbor_updated.local_cost)},
              neighbor_id_{etl::move(neighbor_updated.neighbor_id)},
              neighbor_cost_{etl::move(neighbor_updated.neighbor_cost)},
              link_cost_{etl::move(neighbor_updated.link_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(local_cost_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(neighbor_id_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(neighbor_cost_.serialize(buffer));
            return link_cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + local_cost_.serialized_length() +
                neighbor_id_.serialized_length() + neighbor_cost_.serialized_length() +
                link_cost_.serialized_length();
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
        AsyncNodeNotificationTypeSerializer notification_type_{NodeNotificationType::NeighborRemoved
        };
        node::AsyncNodeIdSerializer neighbor_id_;

      public:
        explicit AsyncNeighborRemovedSerializer(const NeighborRemoved &neighbor_removed)
            : neighbor_id_{etl::move(neighbor_removed.neighbor_id)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return neighbor_id_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + neighbor_id_.serialized_length();
        }
    };

    using NodeNotification = etl::variant<SelfUpdated, NeighborUpdated, NeighborRemoved>;

    class AsyncNodeNotificationSerializer {
        nb::ser::Bin<uint8_t> frame_type_{static_cast<uint8_t>(FrameType::NodeNotification)};
        nb::ser::Bin<uint8_t> notification_type_;
        nb::ser::Union<
            AsyncSelfUpdateSerializer,
            AsyncNeighborUpdatedSerializer,
            AsyncNeighborRemovedSerializer>
            notification_;

        static NodeNotificationType notification_type(const NodeNotification &n) {
            return etl::visit(
                util::Visitor{
                    [](const SelfUpdated &) { return NodeNotificationType::SelfUpdated; },
                    [](const NeighborUpdated &) { return NodeNotificationType::NeighborUpdated; },
                    [](const NeighborRemoved &) { return NodeNotificationType::NeighborRemoved; },
                },
                n
            );
        }

      public:
        explicit AsyncNodeNotificationSerializer(const NodeNotification &notification)
            : notification_type_{static_cast<uint8_t>(notification_type(notification))},
              notification_{notification} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return notification_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return frame_type_.serialized_length() + notification_type_.serialized_length() +
                notification_.serialized_length();
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
        node::Cost local_cost
    ) {
        return etl::visit<NodeNotification>(
            util::Visitor{
                [](const notification::SelfUpdated &self_updated) {
                    return SelfUpdated::from_local(self_updated);
                },
                [local_cost](const notification::NeighborUpdated &neighbor_updated) {
                    return NeighborUpdated::from_local(neighbor_updated, local_cost);
                },
                [](const notification::NeighborRemoved &neighbor_removed) {
                    return NeighborRemoved::from_local(neighbor_removed);
                },
            },
            notification
        );
    }
} // namespace net::observer
