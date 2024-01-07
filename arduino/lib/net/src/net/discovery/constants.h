#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::discovery {
    constexpr inline util::Duration DISCOVERY_FIRST_RESPONSE_TIMEOUT =
        util::Duration::from_seconds(10);
    constexpr inline uint8_t DISCOVERY_BETTER_RESPONSE_TIMEOUT_RATE = 1;
    constexpr inline util::Duration DISCOVER_INTERVAL = util::Duration::from_millis(25);
    constexpr inline uint8_t MAX_CONCURRENT_DISCOVERIES = 4;

    constexpr inline uint8_t FRAME_ID_CACHE_SIZE = 8;
    constexpr inline uint8_t FRAME_DELAY_POOL_SIZE = 4;

    constexpr inline uint8_t MAX_DISCOVERY_CACHE_ENTRIES = 4;
    constexpr inline auto DISCOVERY_CACHE_EXPIRATION = util::Duration::from_seconds(10);
    constexpr inline auto DISCOVERY_CACHE_EXPIRATION_CHECK_INTERVAL =
        util::Duration::from_seconds(1);
} // namespace net::discovery
