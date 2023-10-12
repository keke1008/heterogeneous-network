#pragma once

#include "./address.h"
#include <net/frame/service.h>
#include <stdint.h>

namespace net::link {
    enum class MediaType : uint8_t {
        UHF,
        Wifi,
        Serial,
    };

    struct Frame {
        frame::ProtocolNumber protocol_number;
        Address source;
        Address destination;
        uint8_t length;
        frame::FrameBufferReader reader;
    };
} // namespace net::link
