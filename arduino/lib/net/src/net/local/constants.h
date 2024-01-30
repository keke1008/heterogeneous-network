#pragma once

#include <util/time.h>

namespace net::local {
    inline constexpr util::Duration DYNAMIC_COST_UPDATE_INTERVAL = util::Duration::from_seconds(30);
} // namespace net::local
