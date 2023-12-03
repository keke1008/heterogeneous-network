#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::observer {
    constexpr inline uint8_t FRAME_ID_CACHE_SIZE = 8;

    constexpr inline auto DELETE_NODE_SUBSCRIPTION_TIMEOUT = util::Duration::from_seconds(60);
} // namespace net::observer
