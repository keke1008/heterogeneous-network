#pragma once

#include <net/link.h>
#include <stdint.h>

namespace net::neighbor {
    constexpr uint8_t MAX_MEDIA_PER_NODE = link::MAX_MEDIA_PORT;
    constexpr uint8_t MAX_NEIGHBOR_NODE_COUNT = 10;
    constexpr uint8_t MAX_NEIGHBOR_LIST_CURSOR_COUNT = 5;
    constexpr uint8_t MAX_NEIGNBOR_FRAME_DELAY_POOL_SIZE = 4;
    constexpr util::Duration SEND_HELLO_INTERVAL = util::Duration::from_seconds(10);
    constexpr util::Duration NEIGHBOR_EXPIRATION_TIMEOUT = SEND_HELLO_INTERVAL * 4;
    constexpr util::Duration CHECK_NEIGHBOR_EXPIRATION_INTERVAL = SEND_HELLO_INTERVAL * 2;
} // namespace net::neighbor
