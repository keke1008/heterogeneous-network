#pragma once

#include "./generic.h"

namespace net::link::wifi {
    class StartUdpServer : public Control<255, ResponseType::OK, ResponseType::ERROR> {
      public:
        explicit StartUdpServer(nb::Promise<bool> &&promise, uint16_t local_port)
            : Control{
                  etl::move(promise),
                  R"(AT+CIPSTART="UDP","0.0.0.0",)",
                  nb::buf::FormatDecimal<uint16_t>(local_port),

                  // mode=2とするとremote portの値はフレーム受信時に動的に変わるので，
                  // ダミーの値(12345)を入れておく
                  ",12345,2\r\n",
              } {}
    };
} // namespace net::link::wifi
