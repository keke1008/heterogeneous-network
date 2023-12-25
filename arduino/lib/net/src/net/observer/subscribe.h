#pragma once

#include "./constants.h"
#include "./frame.h"
#include <nb/time.h>
#include <net/frame.h>
#include <net/node.h>
#include <net/routing.h>

namespace net::observer {
    class ReceiveNodeSubscriptionFrameTask {
        routing::RoutingFrame frame_;
        AsyncFrameTypeDeserializer frame_type_;
        AsyncNodeSubscriptionFrameDeserializer node_subscription_frame_;

      public:
        explicit ReceiveNodeSubscriptionFrameTask(routing::RoutingFrame &&frame)
            : frame_{etl::move(frame)} {}

        inline const routing::RoutingFrame &frame() const {
            return frame_;
        }

        nb::Poll<etl::optional<NodeSubscriptionFrame>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.payload.deserialize(frame_type_));
            if (result != nb::DeserializeResult::Ok) {
                return etl::optional<NodeSubscriptionFrame>{};
            }

            result = POLL_UNWRAP_OR_RETURN(frame_.payload.deserialize(node_subscription_frame_));
            return result == nb::DeserializeResult::Ok
                ? etl::optional(node_subscription_frame_.result())
                : etl::nullopt;
        }
    };

    class Subscriber {
        nb::Delay expiration_;
        node::Destination destination_;

      public:
        explicit inline Subscriber(util::Time &time, const node::Destination &destination)
            : expiration_{time, DELETE_NODE_SUBSCRIPTION_TIMEOUT},
              destination_{destination} {}

        inline const node::Destination &destination() const {
            return destination_;
        }

        inline void try_update_expiration(util::Time &time, const node::Destination &destination) {
            if (destination == destination_) {
                expiration_ = nb::Delay(time, DELETE_NODE_SUBSCRIPTION_TIMEOUT);
            }
        }

        inline bool is_expired(util::Time &time) const {
            return expiration_.poll(time).is_ready();
        }
    };

    class SubscriberStorage {
        etl::optional<Subscriber> subscriber_;

      public:
        inline const etl::optional<etl::reference_wrapper<const node::Destination>>
        observer() const {
            return subscriber_.has_value() ? etl::optional(etl::cref(subscriber_->destination()))
                                           : etl::nullopt;
        }

        inline void update_subscriber(util::Time &time, const node::Destination &destination) {
            if (subscriber_.has_value()) {
                subscriber_->try_update_expiration(time, destination);
            } else {
                subscriber_ = Subscriber(time, destination);
            }
        }

        void execute(util::Time &time) {
            if (subscriber_.has_value() && subscriber_->is_expired(time)) {
                subscriber_ = etl::nullopt;
            }
        }
    };

    class SubscribeService {
        SubscriberStorage subscriber_;
        etl::optional<ReceiveNodeSubscriptionFrameTask> receive_frame_task_;

      public:
        inline const etl::optional<etl::reference_wrapper<const node::Destination>>
        observer() const {
            return subscriber_.observer();
        }

        template <nb::AsyncReadableWritable RW>
        inline void execute(
            util::Time &time,
            routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE, FRAME_DELAY_POOL_SIZE> &socket
        ) {
            subscriber_.execute(time);

            if (!receive_frame_task_.has_value()) {
                nb::Poll<routing::RoutingFrame> poll_frame = socket.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return;
                }
                receive_frame_task_.emplace(etl::move(poll_frame.unwrap()));
            }

            const auto &poll_frame = receive_frame_task_->execute();
            if (poll_frame.is_pending()) {
                return;
            }

            if (poll_frame.unwrap().has_value()) {
                const auto &source = receive_frame_task_->frame().source;
                subscriber_.update_subscriber(time, node::Destination(source));
            }
            receive_frame_task_.reset();
        }
    };
} // namespace net::observer
