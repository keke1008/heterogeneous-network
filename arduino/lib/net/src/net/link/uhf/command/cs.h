#pragma once

#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/serde.h>

namespace net::link::uhf {
    class CSExecutor {
        AsyncFixedExecutor<nb::ser::Empty, 2> executor_;

      public:
        CSExecutor() : executor_{'C', 'S', nb::ser::Empty{}} {}

        template <nb::de::AsyncReadable R>
        nb::Poll<bool> poll(R &r) {
            auto span = POLL_UNWRAP_OR_RETURN(executor_.poll(r));
            const bool enabled = span[0] == 'E' && span[1] == 'N';
            return enabled;
        }
    };
} // namespace net::link::uhf
