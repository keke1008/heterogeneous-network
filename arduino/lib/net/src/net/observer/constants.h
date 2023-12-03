#pragma once

#include <stdint.h>
#include <util/time.h>

namespace net::observer {
    constexpr inline auto DELETE_NODE_SUBSCRIPTION_TIMEOUT = util::Duration::from_seconds(60);
}
