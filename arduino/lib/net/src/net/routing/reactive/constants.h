#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::routing::reactive {
    constexpr inline util::TimeInt DISCOVERY_FIRST_RESPONSE_TIMEOUT_MS = 3000;
    constexpr inline util::TimeInt DISCOVERY_BETTER_RESPONSE_TIMEOUT_MS = 1000;
    constexpr inline uint8_t MAX_ROUTE_CACHE_ENTRIES = 8;
    constexpr inline uint8_t FRAME_ID_CACHE_SIZE = 8;
} // namespace net::routing::reactive
