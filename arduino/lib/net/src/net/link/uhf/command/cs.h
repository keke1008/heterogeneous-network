#pragma once

#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace net::link::uhf {
    class CSExecutor {
        FixedExecutor<0, 2> executor_;

      public:
        CSExecutor() : executor_{'@', 'C', 'S', '\r', '\n'} {}

        nb::Poll<bool> poll(nb::stream::ReadableWritableStream &stream) {
            auto span = POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            const bool enabled = span[0] == 'E' && span[1] == 'N';
            return enabled;
        }
    };
} // namespace net::link::uhf
