#pragma once

#include "../response.h"
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>

namespace net::link::wifi {
    template <typename Command>
    class AsyncControl {
        nb::Promise<bool> promise_;
        Command command_;
        AsyncResponseTypeDeserializer response_;

      public:
        template <typename... Args>
        explicit AsyncControl(nb::Promise<bool> &&promise, Args &&...args)
            : promise_{etl::move(promise)},
              command_{etl::forward<Args>(args)...} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(command_.serialize(rw));

            while (true) {
                POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));
                promise_.set_value(is_success_response(response_.result()));
                return nb::ready();
            }
        }
    };
} // namespace net::link::wifi
