#pragma once

#include <stdint.h>

namespace net::protocol {
    enum class ProtocolNumber : uint8_t {
        NoProtocol = 0x00,
        Neighbor = 0x01,
    };
} // namespace net::protocol
