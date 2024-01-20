#pragma once

#include <net/neighbor.h>
#include <stdint.h>

namespace net::tunnel {
    constexpr uint8_t FRAME_DELAY_POOL_SIZE = 8;
    constexpr inline neighbor::NeighborSocketConfig SOCKET_CONFIG{.do_delay = true};
}; // namespace net::tunnel
