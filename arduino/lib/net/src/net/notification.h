#pragma once

#include <etl/circular_buffer.h>
#include <etl/variant.h>
#include <nb/serde.h>
#include <net/link.h>
#include <net/node.h>

namespace net::notification {
    struct SelfUpdated {
        node::OptionalClusterId cluster_id;
        node::Cost cost;
    };

    struct NeighborUpdated {
        node::NodeId neighbor_id;
        node::Cost link_cost;
    };

    struct NeighborRemoved {
        node::NodeId neighbor_id;
    };

    struct FrameReceived {};

    using LocalNotification =
        etl::variant<SelfUpdated, NeighborUpdated, NeighborRemoved, FrameReceived>;

    inline constexpr uint8_t MAX_NOTIFICATION_BUFFER_SIZE = 8;

    class NotificationService {
        etl::circular_buffer<LocalNotification, MAX_NOTIFICATION_BUFFER_SIZE> notification_buffer_;

      public:
        inline void notify(const LocalNotification &notification) {
            if (notification_buffer_.full()) {
                LOG_INFO(FLASH_STRING("Notification buffer is full"));
            } else {
                notification_buffer_.push(notification);
            }
        }

        inline nb::Poll<LocalNotification> poll_notification() {
            if (notification_buffer_.empty()) {
                return nb::pending;
            }
            auto front = notification_buffer_.front();
            notification_buffer_.pop();
            return front;
        }
    };
} // namespace net::notification
