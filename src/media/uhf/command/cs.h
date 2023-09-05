#pragma once

#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    class CSExecutor {
        FixedExecutor<0, 2> executor_;

      public:
        CSExecutor() : executor_{'@', 'C', 'S', '\r', '\n'} {}

        template <typename Serial>
        nb::Poll<bool> poll(Serial &serial) {
            auto &body = POLL_UNWRAP_OR_RETURN(executor_.poll(serial)).get();
            bool enabled = body.template get<0>() == 'E' && body.template get<1>() == 'N';
            return nb::Ready{enabled};
        }
    };
} // namespace media::uhf
