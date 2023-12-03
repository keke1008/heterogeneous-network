#pragma once

#include "./constants.h"
#include <nb/time.h>
#include <net/node.h>
#include <net/routing.h>
#include <tl/set.h>

namespace net::observer {
    class Subscriber {
        node::NodeId id_;
        nb::Delay expiration_;

      public:
        explicit inline Subscriber(node::NodeId id, util::Time &time)
            : id_{id},
              expiration_{time, DELETE_NODE_SUBSCRIPTION_TIMEOUT} {}

        inline const node::NodeId &id() const {
            return id_;
        }

        inline void try_update_expiration(util::Time &time, const node::NodeId &id) {
            if (id == id_) {
                expiration_ = nb::Delay(time, DELETE_NODE_SUBSCRIPTION_TIMEOUT);
            }
        }

        inline bool is_expired(util::Time &time) const {
            return expiration_.poll(time).is_ready();
        }
    };

    class SubscribeServier {
        etl::optional<Subscriber> subscriber_;

      public:
        const etl::optional<etl::reference_wrapper<const node::NodeId>> observer_id() const {
            return subscriber_.has_value() ? etl::optional(etl::cref(subscriber_->id()))
                                           : etl::nullopt;
        }

        inline void on_frame_received(util::Time &time, const routing::RoutingFrame &frame) {
            if (subscriber_.has_value()) {
                subscriber_->try_update_expiration(time, frame.header.sender_id);
            } else {
                subscriber_ = Subscriber(frame.header.sender_id, time);
            }
        }

        inline void execute(util::Time &time) {
            if (subscriber_.has_value() && subscriber_->is_expired(time)) {
                subscriber_ = etl::nullopt;
            }
        }
    };
} // namespace net::observer
