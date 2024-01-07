#pragma once

#include <stdint.h>

namespace net::routing {
    struct RoutingSocketEvent {
        uint8_t frame_received : 1;

        static constexpr RoutingSocketEvent empty() {
            return {
                .frame_received = 0,
            };
        }

        inline void set_frame_received() {
            frame_received = 1;
        }
    };
} // namespace net::routing
