#pragma once

#include "../frame.h"

namespace media::wifi {
    struct GotLocalIp {};

    struct DisconnectAp {};

    struct ReceiveFrame {
        WifiFrame frame;
    };

    enum class PassiveMessage : uint8_t {
        OK,
        Error,
        Fail,
        SendOk,
        SendFail,
        SendPrompt,
    };

    struct ReceivePassiveMessage {
        PassiveMessage type;
    };

    using WifiEvent = etl::variant<GotLocalIp, DisconnectAp, ReceiveFrame, ReceivePassiveMessage>;
} // namespace media::wifi
