#pragma once

#include "../event.h"
#include <nb/serde.h>
#include <util/span.h>

namespace net::link::wifi {
    class WifiMessageHandler {
        // "DISCONNECT\r\n" | "GOT IP\r\n"
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<12> deserializer_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<etl::optional<WifiEvent>> execute(R &readable) {
            auto result = POLL_UNWRAP_OR_RETURN(deserializer_.deserialize(readable));
            if (result != nb::DeserializeResult::Ok) {
                return etl::optional<WifiEvent>{etl::nullopt};
            }

            auto line = util::as_str(deserializer_.result());
            if (line == "DISCONNECT\r\n") {
                return etl::optional<WifiEvent>(DisconnectAp{});
            }
            if (line == "GOT IP\r\n") {
                return etl::optional<WifiEvent>(GotLocalIp{});
            }
            return etl::optional<WifiEvent>{etl::nullopt};
        }
    };
}; // namespace net::link::wifi
