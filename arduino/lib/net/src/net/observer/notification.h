#pragma once

#include "./constants.h"
#include "./frame.h"
#include <nb/poll.h>
#include <net/notification.h>
#include <net/routing.h>

namespace net::observer {
    class SendNotificationFrameTask {
        node::Destination observer_;
        AsyncNodeNotificationSerializer serializer_;
        etl::optional<frame::FrameBufferReader> reader_;

      public:
        explicit SendNotificationFrameTask(
            const node::Destination &observer,
            const notification::LocalNotification &local_notification,
            node::OptionalClusterId local_cluster_id,
            node::Cost local_cost
        )
            : observer_{observer},
              serializer_{from_local_notification(local_notification, local_cluster_id, local_cost)
              } {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            routing::RoutingSocket<RW, FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!reader_.has_value()) {
                uint8_t length = serializer_.serialized_length();
                frame::FrameBufferWriter writer = POLL_MOVE_UNWRAP_OR_RETURN(
                    socket.poll_frame_writer(fs, lns, rand, observer_, length)
                );
                writer.serialize_all_at_once(serializer_);
                reader_ = writer.create_reader();
            }

            POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_send_frame(observer_, etl::move(*reader_)));
            return nb::ready();
        }
    };

    class NotificationService {
        etl::optional<SendNotificationFrameTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            notification::NotificationService &ns,
            routing::RoutingSocket<RW, FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand,
            etl::optional<etl::reference_wrapper<const node::Destination>> observer
        ) {
            while (true) {
                if (task_.has_value()) {
                    nb::Poll<void> poll_execute = task_->execute(fs, lns, socket, time, rand);
                    if (poll_execute.is_pending()) {
                        return;
                    }
                    task_.reset();
                }

                if (!observer.has_value()) {
                    return;
                }

                const auto &poll_info = lns.poll_info();
                if (poll_info.is_pending()) {
                    return;
                }

                auto poll_notification = ns.poll_notification();
                if (poll_notification.is_pending()) {
                    return;
                }

                const auto &info = poll_info.unwrap();
                task_.emplace(
                    observer->get(), poll_notification.unwrap(), info.source.cluster_id, info.cost
                );
            }
        }
    };
} // namespace net::observer
