#pragma once

#include "../link.h"
#include "./generic.h"
#include <nb/stream.h>
#include <net/link/address.h>

namespace net::link::wifi {
    class StartTcpConnection final
        : public Control<49, message::ResponseType::OK, message::ResponseType::NEVER> {
      public:
        explicit StartTcpConnection(
            nb::Promise<bool> &&promise,
            LinkId link_id,
            IPv4Address remote_ip,
            uint16_t remote_port
        )
            : Control{
                  etl::move(promise),
                  R"(AT+CIPSTART=)",
                  link_id_to_byte(link_id),
                  R"(,"TCP",")",
                  remote_ip,
                  R"(",)",
                  nb::stream::FormatDecimal(remote_port),
                  "\r\n",
              } {}
    };
} // namespace net::link::wifi
