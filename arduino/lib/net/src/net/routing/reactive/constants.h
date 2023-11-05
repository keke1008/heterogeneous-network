#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::routing::reactive {
    constexpr inline util::Duration DISCOVERY_FIRST_RESPONSE_TIMEOUT =
        util::Duration::from_millis(3000);
    constexpr inline util::Duration DISCOVERY_BETTER_RESPONSE_TIMEOUT =
        util::Duration::from_millis(1000);
    constexpr inline util::Duration DISCOVER_INTERVAL = util::Duration::from_millis(250);
    constexpr inline uint8_t MAX_CONCURRENT_DISCOVERIES = 4;
    constexpr inline uint8_t MAX_ROUTE_CACHE_ENTRIES = 8;
    constexpr inline uint8_t FRAME_ID_CACHE_SIZE = 8;
} // namespace net::routing::reactive
