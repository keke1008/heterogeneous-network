#pragma once

#include <stdint.h>

namespace net::link::wifi {
    enum class LinkId : uint8_t {
        Link0 = 0,
        Link1 = 1,
        Link2 = 2,
        Link3 = 3,
        Link4 = 4,
    };

    inline uint8_t link_id_to_byte(LinkId link_id) {
        return static_cast<uint8_t>(link_id) + '0';
    }
} // namespace net::link::wifi
