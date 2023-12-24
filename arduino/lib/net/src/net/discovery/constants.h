#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::discovery {
    constexpr inline uint8_t FRAME_DELAY_POOL_SIZE = 4;
    constexpr inline util::Duration DISCOVERY_FIRST_RESPONSE_TIMEOUT =
        util::Duration::from_millis(1000);
    constexpr inline uint8_t DISCOVERY_BETTER_RESPONSE_TIMEOUT_RATE = 1;
    constexpr inline util::Duration DISCOVER_INTERVAL = util::Duration::from_millis(25);
    constexpr inline uint8_t MAX_CONCURRENT_DISCOVERIES = 4;
    constexpr inline uint8_t MAX_DISCOVERY_CACHE_ENTRIES = 4;
    constexpr inline uint8_t FRAME_ID_CACHE_SIZE = 8;
} // namespace net::discovery
