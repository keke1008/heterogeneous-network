#pragma once

#include <util/time.h>

namespace net::rpc {
    constexpr inline util::Duration RESPONSE_TIMEOUT = util::Duration::from_seconds(5);
}
