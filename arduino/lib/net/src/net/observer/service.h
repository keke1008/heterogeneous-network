#pragma once

#include "./notification.h"
#include "./subscribe.h"

namespace net::observer {
    class ReceiveFrameTask {
        routing::RoutingFrame frame_;
        AsyncFrameTypeDeserializer frame_type_;

      public:
        explicit ReceiveFrameTask(routing::RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        inline const routing::RoutingFrame &frame() const {
            return frame_;
        }

        nb::Poll<etl::optional<FrameType>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.payload.deserialize(frame_type_));
            return result == nb::DeserializeResult::Ok
                ? etl::optional<FrameType>(frame_type_.result())
                : etl::nullopt;
        }
    };

    template <nb::AsyncReadableWritable RW>
    class ObserverService {
        NotificationService notification_service_;
        SubscribeServier subscribe_servier_;
        routing::RoutingSocket<RW> socket_;
        etl::optional<ReceiveFrameTask> receive_frame_task_;

      public:
        explicit ObserverService(link::LinkService<RW> &link_service)
            : notification_service_{},
              subscribe_servier_{},
              socket_{link_service.open(frame::ProtocolNumber::Observer)} {}

        void execute(
            frame::FrameService &fs,
            notification::NotificationService &ns,
            routing::RoutingService<RW> &rs,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(rs, time, rand);
            notification_service_.execute(
                fs, ns, rs, socket_, time, rand, subscribe_servier_.observer_id()
            );

            if (!receive_frame_task_.has_value()) {
                nb::Poll<routing::RoutingFrame> poll_frame = socket_.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return;
                }
                receive_frame_task_.emplace(etl::move(poll_frame.unwrap()));
            }

            auto poll_frame_type = receive_frame_task_->execute();
            if (poll_frame_type.is_pending()) {
                return;
            }
            if (poll_frame_type.unwrap() == FrameType::NodeSubscription) {
                subscribe_servier_.on_frame_received(time, receive_frame_task_->frame());
            }
            receive_frame_task_.reset();
        }
    };
} // namespace net::observer
