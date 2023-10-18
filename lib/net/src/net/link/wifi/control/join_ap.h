#pragma once

#include "./generic.h"
#include <nb/poll.h>
#include <nb/stream.h>

namespace net::link::wifi {
    class JoinAp final : public Control<112, ResponseType::OK, ResponseType::FAIL> {
      public:
        explicit JoinAp(
            nb::Promise<bool> &&promise,
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password
        )
            : Control{
                  etl::move(promise), "AT+CWJAP=", '"', ssid, R"(",")", password, '"', CRLF,
              } {}
    };
} // namespace net::link::wifi
