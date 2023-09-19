#pragma once

#include "../command/sn.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class GetSerialNumberTask {
        SNExecutor executor_;
        nb::Promise<SerialNumber> promise_;

      public:
        inline GetSerialNumberTask(nb::Promise<SerialNumber> &&promise)
            : executor_{},
              promise_{std::move(promise)} {}

        inline nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            auto serial = POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            promise_.set_value(serial);
            return nb::ready();
        }
    };
} // namespace net::link::uhf
