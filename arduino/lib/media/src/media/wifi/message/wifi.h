#pragma once

#include "./notification.h"
#include <nb/lock.h>
#include <nb/serde.h>
#include <util/span.h>

namespace media::wifi {
    template <nb::AsyncReadable R>
    class WifiMessageHandler {
        // "DISCONNECT\r\n" | "GOT IP\r\n"
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<12> deserializer_;
        nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> readable_;

      public:
        explicit WifiMessageHandler(
            nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> &&readable
        )
            : readable_{etl::move(readable)} {}

        nb::Poll<etl::optional<WifiEvent>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(deserializer_.deserialize(*readable_->get()));
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
}; // namespace media::wifi
