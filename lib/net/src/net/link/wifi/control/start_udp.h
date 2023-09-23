#pragma once

#include "../link.h"
#include "./generic.h"
#include <nb/stream.h>
#include <net/link/address.h>

namespace net::link::wifi {
    namespace {
        constexpr inline auto port_to_buffer_writer(uint16_t port) {
            return [port](auto span) { return serde::dec::serialize(span, port); };
        }
    }; // namespace

    class StartUdpConnection final : public Control<49, ResponseType::OK, ResponseType::NEVER> {
      public:
        explicit StartUdpConnection(
            nb::Promise<bool> &&promise,
            LinkId link_id,
            IPv4Address remote_ip,
            uint16_t remote_port,
            uint16_t local_port
        )
            : Control{
                  etl::move(promise),
                  "AT+CIPSTART=",
                  link_id_to_byte(link_id),
                  R"(,"UDP",")",
                  remote_ip,
                  "\",",
                  port_to_buffer_writer(remote_port),
                  ',',
                  port_to_buffer_writer(local_port),
                  "\r\n"} {}
    };
} // namespace net::link::wifi
