#pragma once

#include "./generic.h"

namespace media::wifi {
    struct CloseUdpServer : public AsyncControl<nb::ser::AsyncStaticSpanSerializer> {
        inline CloseUdpServer(nb::Promise<bool> &&promise)
            : AsyncControl{etl::move(promise), "AT+CIPCLOSE\r\n"} {}
    };
} // namespace media::wifi
