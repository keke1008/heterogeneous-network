#pragma once

#include "./generic.h"

namespace net::link::wifi {
    struct CloseUdpServer : public Control<13, ResponseType::OK, ResponseType::ERROR> {
        inline CloseUdpServer(nb::Promise<bool> &&promise)
            : Control{etl::move(promise), "AT+CIPCLOSE\r\n"} {}
    };
} // namespace net::link::wifi
