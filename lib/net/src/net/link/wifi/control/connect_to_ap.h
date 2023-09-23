#pragma once

#include "./generic.h"
#include <nb/poll.h>
#include <nb/stream.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

namespace net::link::wifi {
    class ConnectToAp final
        : public Control<112, message::ResponseType::OK, message::ResponseType::FAIL> {
      public:
        explicit ConnectToAp(
            nb::Promise<bool> &&promise,
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password
        )
            : Control{
                  etl::move(promise), "AT+CWJAP=", '"', ssid, R"(",")", password, '"', CRLF,
              } {}
    };
} // namespace net::link::wifi
