#pragma once

#include "../common.h"
#include "./common.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class RIExecutor {
        AsyncFixedExecutor<nb::ser::AsyncStaticSpanSerializer, 2> executor_{'R', 'I', "ON"};

      public:
        RIExecutor() = default;

        template <nb::AsyncReadable R>
        nb::Poll<void> poll(R &r) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(r));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
