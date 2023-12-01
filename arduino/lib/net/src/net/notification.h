#pragma once

#include <etl/circular_buffer.h>
#include <etl/variant.h>
#include <nb/serde.h>
#include <net/link.h>
#include <net/routing.h>

namespace net::notification {
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
        link::Cost cost;
    };

    class AsyncSelfUpdateSerializer {
        AsyncNodeNotificationTypeSerializer notification_type_{NodeNotificationType::SelfUpdated};
        link::AsyncCostSerializer cost_;

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
        routing::NodeId neighbor_id;
        link::Cost link_cost;
        link::Cost neighbor_cost;
    };

    class AsyncNeighborUpdatedSerializer {
        AsyncNodeNotificationTypeSerializer notification_type_{
            NodeNotificationType::NeighborUpdated};
        routing::AsyncNodeIdSerializer node_id_;
        link::AsyncCostSerializer link_cost_;
        link::AsyncCostSerializer neighbor_cost_;

      public:
        explicit AsyncNeighborUpdatedSerializer(const NeighborUpdated &neighbor_updated)
            : node_id_{etl::move(neighbor_updated.neighbor_id)},
              link_cost_{etl::move(neighbor_updated.link_cost)},
              neighbor_cost_{etl::move(neighbor_updated.neighbor_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(node_id_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(link_cost_.serialize(buffer));
            return neighbor_cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + node_id_.serialized_length() +
                link_cost_.serialized_length() + neighbor_cost_.serialized_length();
        }
    };

    struct NeighborRemoved {
        routing::NodeId neighbor_id;
    };

    class AsyncNeighborRemovedSerializer {
        AsyncNodeNotificationTypeSerializer notification_type_{
            NodeNotificationType::NeighborRemoved};
        routing::AsyncNodeIdSerializer node_id_;

      public:
        explicit AsyncNeighborRemovedSerializer(const NeighborRemoved &neighbor_removed)
            : node_id_{etl::move(neighbor_removed.neighbor_id)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return node_id_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + node_id_.serialized_length();
        }
    };

    using Notification = etl::variant<SelfUpdated, NeighborUpdated, NeighborRemoved>;

    class AsyncNodeNotificationSerializer {
        using Serializer = nb::ser::Union<
            AsyncSelfUpdateSerializer,
            AsyncNeighborUpdatedSerializer,
            AsyncNeighborRemovedSerializer>;

        AsyncNodeNotificationTypeSerializer notification_type_;
        Serializer serializer_;

        static NodeNotificationType notification_type(const Notification &notification) {
            return etl::visit<NodeNotificationType>(
                util::Visitor{
                    [](const SelfUpdated &) { return NodeNotificationType::SelfUpdated; },
                    [](const NeighborUpdated &) { return NodeNotificationType::NeighborUpdated; },
                    [](const NeighborRemoved &) { return NodeNotificationType::NeighborRemoved; },
                },
                notification
            );
        }

      public:
        explicit AsyncNodeNotificationSerializer(const Notification &notification)
            : notification_type_{notification_type(notification)},
              serializer_{notification} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return serializer_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + serializer_.serialized_length();
        }
    };

    inline constexpr uint8_t MAX_NOTIFICATION_BUFFER_SIZE = 8;

    class NotificationService {
        etl::circular_buffer<Notification, MAX_NOTIFICATION_BUFFER_SIZE> notification_buffer_;

      public:
        inline void notify(const Notification &notification) {
            if (!notification_buffer_.full()) {
                notification_buffer_.push(notification);
            }
        }

        inline nb::Poll<Notification> poll_notification() {
            if (notification_buffer_.empty()) {
                return nb::pending;
            }
            auto front = notification_buffer_.front();
            notification_buffer_.pop();
            return front;
        }
    };
} // namespace net::notification
