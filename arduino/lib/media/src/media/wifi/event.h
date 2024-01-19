#pragma once

#include "./frame.h"

namespace media::wifi {
    struct GotLocalIp {};

    struct DisconnectAp {};

    struct SentDataFrame {
        UdpAddress destination;
    };

    struct ReceiveDataFrame {
        WifiDataFrame frame;
    };

    using WifiEvent = etl::variant<GotLocalIp, DisconnectAp, SentDataFrame, ReceiveDataFrame>;
} // namespace media::wifi
