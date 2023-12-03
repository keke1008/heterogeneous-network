#pragma once

#include "./constants.h"
#include "./frame.h"
#include "tl/set.h"
#include <nb/poll.h>
#include <net/notification.h>
#include <net/routing.h>

namespace net::observer {
    class SendNotificationFrameTask {
        node::NodeId observer_id_;
        AsyncNodeNotificationSerializer serializer_;
        etl::optional<frame::FrameBufferReader> reader_;

      public:
        explicit SendNotificationFrameTask(
            const node::NodeId &observer_id,
            const notification::Notification &notification
        )
            : observer_id_{observer_id},
              serializer_{notification} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            frame::FrameService &fs,
            routing::RoutingService<RW> &rs,
            routing::RoutingSocket<RW> &socket,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!reader_.has_value()) {
                uint8_t length = serializer_.serialized_length();
                frame::FrameBufferWriter writer = POLL_MOVE_UNWRAP_OR_RETURN(
                    socket.poll_frame_writer(fs, rs, observer_id_, length)
                );
                writer.serialize_all_at_once(serializer_);
                reader_ = writer.create_reader();
            }

            POLL_MOVE_UNWRAP_OR_RETURN(
                socket.poll_send_frame(rs, observer_id_, etl::move(*reader_), time, rand)
            );
            return nb::ready();
        }
    };

    class NotificationService {
        etl::optional<SendNotificationFrameTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            frame::FrameService &fs,
            notification::NotificationService &ns,
            routing::RoutingService<RW> &rs,
            routing::RoutingSocket<RW> &socket,
            util::Time &time,
            util::Rand &rand,
            etl::optional<etl::reference_wrapper<const node::NodeId>> observer_id
        ) {
            while (true) {
                if (task_.has_value()) {
                    nb::Poll<void> poll_execute = task_->execute(fs, rs, socket, time, rand);
                    if (poll_execute.is_pending()) {
                        return;
                    }
                }

                if (!observer_id.has_value()) {
                    return;
                }

                auto poll_notification = ns.poll_notification();
                if (poll_notification.is_pending()) {
                    task_.reset();
                    return;
                }

                task_.emplace(observer_id->get(), poll_notification.unwrap());
            }
        }
    };
} // namespace net::observer
