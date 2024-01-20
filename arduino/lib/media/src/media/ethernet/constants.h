#pragma once

#include <stdint.h>
#include <util/time.h>

namespace media::ethernet {
    constexpr auto CHECK_LINK_UP_INTERVAL = util::Duration::from_seconds(5);
    constexpr uint16_t UDP_PORT = 8888;
    constexpr uint8_t MAX_TRANSFERRABLE_BYTES_AT_ONCE = 64;
} // namespace media::ethernet
