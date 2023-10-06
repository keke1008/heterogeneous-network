#pragma once

#include <stdint.h>

namespace net::frame {
    enum class ProtocolNumber : uint8_t {
        NoProtocol = 0x00,
        RoutingNeighbor = 0x01,
        RoutingLinkState = 0x02,
    };
} // namespace net::frame
