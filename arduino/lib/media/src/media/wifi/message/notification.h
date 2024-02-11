#pragma once

#include "../frame.h"
#include <nb/lock.h>

namespace media::wifi {
    struct GotLocalIp {};

    struct DisconnectAp {};

    struct ReceiveFrame {
        WifiFrame frame;
    };

    using WifiEvent = etl::variant<GotLocalIp, DisconnectAp, ReceiveFrame>;

    enum class WifiResponseMessage : uint8_t {
        Ok,
        Error,
        Fail,
        SendOk,
        SendFail,
        SendPrompt,
    };

    enum class WifiMessageReportType : uint8_t {
        CIPSTA,
    };

    template <nb::AsyncReadable R>
    struct WifiMessageReport {
        WifiMessageReportType type;
        nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> body;
    };

    template <nb::AsyncReadable R>
    using WifiMessage = etl::variant<WifiResponseMessage, WifiMessageReport<R>>;
} // namespace media::wifi
