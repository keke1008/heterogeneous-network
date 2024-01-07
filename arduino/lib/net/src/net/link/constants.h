#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::link {
    constexpr uint8_t MAX_FRAME_BUFFER_SIZE = 8;
    constexpr util::Duration FRAME_DROP_INTERVAL = util::Duration::from_millis(1000);
    constexpr util::Duration FRAME_EXPIRATION = util::Duration::from_millis(1000);
} // namespace net::link
