#pragma once

#include "../frame.h"

namespace net::routing::worker {
    class AcceptWorker {
        etl::optional<RoutingFrame> frame_;

      public:
        inline nb::Poll<RoutingFrame> poll_frame() {
            return frame_ ? nb::ready(etl::move(*frame_)) : nb::pending;
        }

        inline nb::Poll<void> poll_accept(RoutingFrame &&frame) {
            if (frame_) {
                return nb::pending;
            }

            frame_.emplace(etl::move(frame));
            return nb::ready();
        }

        inline nb::Poll<void> poll_accept_unicast_frame(UnicastRoutingFrame &&frame) {
            if (frame_) {
                return nb::pending;
            }

            frame_.emplace(RoutingFrame(etl::move(frame)));
            return nb::ready();
        }

        inline nb::Poll<void> poll_accept_broadcast_frame(BroadcastRoutingFrame &&frame) {
            if (frame_) {
                return nb::pending;
            }

            frame_.emplace(RoutingFrame(etl::move(frame)));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
