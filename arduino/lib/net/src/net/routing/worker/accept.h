#pragma once

#include "../frame.h"

namespace net::routing::worker {
    class AcceptWorker {
        etl::optional<RoutingFrame> frame_;

      public:
        inline nb::Poll<RoutingFrame> poll_frame() {
            if (!frame_) {
                return nb::pending;
            }

            auto frame = etl::move(*frame_);
            frame_.reset();
            return etl::move(frame);
        }

        inline nb::Poll<void> poll_accept_frame(RoutingFrame &&frame) {
            if (frame_) {
                return nb::pending;
            }

            frame_.emplace(RoutingFrame(etl::move(frame)));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
