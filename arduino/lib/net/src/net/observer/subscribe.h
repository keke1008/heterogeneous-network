#pragma once

#include "./notification.h"
#include "tl/set.h"

namespace net::observer {
    class SubscribeServier {
        tl::Set<routing::NodeId, MAX_OBSERVER_COUNT> observer_ids_;

      public:
        const tl::Set<routing::NodeId, MAX_OBSERVER_COUNT> &observer_ids() const {
            return observer_ids_;
        }

        inline void on_frame_received(const routing::RoutingFrame &frame) {
            observer_ids_.insert(frame.header.sender_id);
        }
    };
} // namespace net::observer
