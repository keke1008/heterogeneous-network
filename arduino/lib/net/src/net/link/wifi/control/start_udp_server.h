#pragma once

#include "./generic.h"

namespace net::link::wifi {
    class StartUdpServer : public Control<255, ResponseType::OK, ResponseType::ERROR> {
      public:
        explicit StartUdpServer(nb::Promise<bool> &&promise, uint16_t port)
            : Control{
                  etl::move(promise),
                  R"(AT+CIPSTART="UDP","0.0.0.0",)",
                  nb::buf::FormatDecimal<uint16_t>(port),
                  ',',
                  nb::buf::FormatDecimal<uint16_t>(port),
                  "\r\n",
              } {}
    };
} // namespace net::link::wifi
