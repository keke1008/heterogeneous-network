#pragma once

#include <etl/circular_buffer.h>
#include <etl/variant.h>
#include <nb/serde.h>
#include <net/link.h>

namespace net::notification {
    enum class NotificationType : uint8_t {
        SelfUpdated,
        NeighborUpdated,
        NeighborRemoved,
    };

    class AsyncNotificationTypeSerializer {
        nb::ser::Bin<uint8_t> serializer_;

      public:
        explicit AsyncNotificationTypeSerializer(NotificationType notification_type)
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
        AsyncNotificationTypeSerializer notification_type_{NotificationType::SelfUpdated};
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
        link::Address neighbor_address;
        link::Cost link_cost;
        link::Cost neighbor_cost;
    };

    class AsyncNeighborUpdatedSerializer {
        AsyncNotificationTypeSerializer notification_type_{NotificationType::NeighborUpdated};
        link::AsyncAddressSerializer address_serializer_;
        link::AsyncCostSerializer link_cost_serializer_;
        link::AsyncCostSerializer neighbor_cost_serializer_;

      public:
        explicit AsyncNeighborUpdatedSerializer(const NeighborUpdated &neighbor_updated)
            : address_serializer_{etl::move(neighbor_updated.neighbor_address)},
              link_cost_serializer_{etl::move(neighbor_updated.link_cost)},
              neighbor_cost_serializer_{etl::move(neighbor_updated.neighbor_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(address_serializer_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(link_cost_serializer_.serialize(buffer));
            return neighbor_cost_serializer_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() +
                address_serializer_.serialized_length() +
                link_cost_serializer_.serialized_length() +
                neighbor_cost_serializer_.serialized_length();
        }
    };

    struct NeighborRemoved {
        link::Address neighbor_address;
    };

    class AsyncNeighborRemovedSerializer {
        AsyncNotificationTypeSerializer notification_type_{NotificationType::NeighborRemoved};
        link::AsyncAddressSerializer address_serializer_;

      public:
        explicit AsyncNeighborRemovedSerializer(const NeighborRemoved &neighbor_removed)
            : address_serializer_{etl::move(neighbor_removed.neighbor_address)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(notification_type_.serialize(buffer));
            return address_serializer_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return notification_type_.serialized_length() + address_serializer_.serialized_length();
        }
    };

    using Notification = etl::variant<SelfUpdated, NeighborUpdated, NeighborRemoved>;

    class AsyncNotificationSerializer {
        using Serializer = nb::ser::Union<
            AsyncSelfUpdateSerializer,
            AsyncNeighborUpdatedSerializer,
            AsyncNeighborRemovedSerializer>;

        AsyncNotificationTypeSerializer notification_type_;
        Serializer serializer_;

        static NotificationType notification_type(const Notification &notification) {
            return etl::visit<NotificationType>(
                util::Visitor{
                    [](const SelfUpdated &) { return NotificationType::SelfUpdated; },
                    [](const NeighborUpdated &) { return NotificationType::NeighborUpdated; },
                    [](const NeighborRemoved &) { return NotificationType::NeighborRemoved; },
                },
                notification
            );
        }

      public:
        explicit AsyncNotificationSerializer(const Notification &notification)
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
