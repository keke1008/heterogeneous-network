#pragma once

#include "./frame.h"

namespace net::link::wifi {
    struct GotLocalIp {};

    struct DisconnectAp {};

    struct SentDataFrame {
        WifiAddress destination;
    };

    struct ReceiveDataFrame {
        WifiDataFrame frame;
    };

    struct ReceiveControlFrame {
        WifiAddress source;
        WifiControlFrame frame;
    };

    using WifiEvent = etl::
        variant<GotLocalIp, DisconnectAp, SentDataFrame, ReceiveDataFrame, ReceiveControlFrame>;
} // namespace net::link::wifi
