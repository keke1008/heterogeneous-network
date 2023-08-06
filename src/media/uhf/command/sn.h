#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    template <typename Serial>
    class SNExecutor {
        FixedExecutor<Serial, 0, 9> executor_;

      public:
        SNExecutor(Serial &&serial) : executor_{etl::move(serial), '@', 'S', 'N', '\r', '\n'} {}

        nb::Poll<SerialNumber> poll() {
            auto &body = POLL_UNWRAP_OR_RETURN(executor_.poll()).get();
            return SerialNumber{body};
        }
    };
} // namespace media::uhf
