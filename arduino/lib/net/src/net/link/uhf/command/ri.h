#pragma once

#include "../common.h"
#include "./common.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class RIExecutor {
        FixedExecutor<2, 2> executor_;

      public:
        RIExecutor() : executor_{'@', 'R', 'I', 'O', 'N', '\r', '\n'} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
