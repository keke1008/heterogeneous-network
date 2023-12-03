#pragma once

#include <net/link.h>
#include <stdint.h>

namespace net::neighbor {
    constexpr uint8_t MAX_MEDIA_PER_NODE = link::MAX_MEDIA_PORT;
    constexpr uint8_t MAX_NEIGHBOR_NODE_COUNT = 10;
} // namespace net::neighbor
