#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::link {
    constexpr uint8_t MAX_FRAME_BUFFER_SIZE = 8;
    constexpr util::Duration FRAME_DROP_INTERVAL = util::Duration::from_seconds(1);
    constexpr util::Duration FRAME_EXPIRATION = util::Duration::from_seconds(5);

    constexpr uint8_t MAX_MEDIA_PER_NODE = 4;
} // namespace net::link
