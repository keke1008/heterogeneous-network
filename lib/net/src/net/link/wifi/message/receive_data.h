#pragma once

#include "../link.h"
#include <stdint.h>

namespace net::link::wifi {
    struct ReceiveDataMessage {
        LinkId link_id;
        uint32_t length;
    };
} // namespace net::link::wifi
