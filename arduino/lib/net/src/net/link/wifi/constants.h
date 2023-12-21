#pragma once

#include <util/time.h>

namespace net::link::wifi {
    constexpr util::Duration DEFAULT_TASK_TIMEOUT = util::Duration::from_millis(1000);
    constexpr util::Duration JOIN_AP_TIMEOUT = util::Duration::from_seconds(20);
} // namespace net::link::wifi
