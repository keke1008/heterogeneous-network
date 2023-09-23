#pragma once

#include "./generic.h"
#include <nb/stream.h>

namespace net::link::wifi {
    class StartTcpServer
        : public Control<50, message::ResponseType::OK, message::ResponseType::NEVER> {
      public:
        explicit StartTcpServer(nb::Promise<bool> &&promise, uint16_t port)
            : Control{
                  etl::move(promise),
                  R"(AT+CIPSERVER=1,)",
                  nb::stream::FormatDecimal(port),
                  "\r\n",
              } {}
    };
} // namespace net::link::wifi
