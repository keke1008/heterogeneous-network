#pragma once

#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    template <typename Serial>
    class CSExecutor {
        FixedExecutor<Serial, 0, 2> executor_;

      public:
        CSExecutor(Serial &&serial) : executor_{etl::move(serial), '@', 'C', 'S', '\r', '\n'} {}

        nb::Poll<bool> poll() {
            auto &body = POLL_UNWRAP_OR_RETURN(executor_.poll()).get();
            bool enabled = body.template get<0>() == 'E' && body.template get<1>() == 'N';
            return nb::Ready{enabled};
        }
    };
} // namespace media::uhf
