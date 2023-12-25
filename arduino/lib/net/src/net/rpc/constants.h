#pragma once

#include <util/time.h>

namespace net::rpc {
    constexpr inline uint8_t FRAME_DELAY_POOL_SIZE = 4;

    constexpr inline util::Duration RESPONSE_TIMEOUT = util::Duration::from_seconds(5);
    constexpr inline util::Duration WIFI_CONNECT_TO_ACCESS_POINT_TIMEOUT =
        util::Duration::from_seconds(15);
} // namespace net::rpc
