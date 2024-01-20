#pragma once

#include "./frame.h"

namespace media::wifi {
    struct GotLocalIp {};

    struct DisconnectAp {};

    struct ReceiveFrame {
        WifiFrame frame;
    };

    using WifiEvent = etl::variant<GotLocalIp, DisconnectAp, ReceiveFrame>;
} // namespace media::wifi
