#pragma once

#include <net/neighbor.h>
#include <stdint.h>
#include <util/time.h>

namespace net::observer {
    constexpr inline uint8_t FRAME_DELAY_POOL_SIZE = 4;
    constexpr inline neighbor::NeighborSocketConfig SOCKET_CONFIG{.do_delay = false};

    constexpr inline auto DELETE_NODE_SUBSCRIPTION_TIMEOUT = util::Duration::from_seconds(60);

    constexpr inline auto NODE_SYNC_INTERVAL = util::Duration::from_seconds(60);
} // namespace net::observer
