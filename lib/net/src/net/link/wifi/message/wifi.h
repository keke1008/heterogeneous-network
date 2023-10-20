#pragma once

#include <nb/stream.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
    enum class WifiEvent : uint8_t {
        GotIp,
        Disconnect,
    };

    class WifiMessageHandler {
        nb::stream::MaxLengthSingleLineWrtableBuffer<12> buffer_; // "DISCONNECT\r\n" | "GOT IP\r\n"

      public:
        nb::Poll<etl::optional<WifiEvent>> execute(nb::stream::ReadableStream &stream) {
            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            if (util::as_str(buffer_.written_bytes()) == "DISCONNECT\r\n") {
                return etl::optional(WifiEvent::Disconnect);
            }
            if (util::as_str(buffer_.written_bytes()) == "GOT IP\r\n") {
                return etl::optional(WifiEvent::GotIp);
            }
            return etl::optional<WifiEvent>{etl::nullopt};
        }
    };
}; // namespace net::link::wifi
