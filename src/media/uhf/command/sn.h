#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    class SNExecutor {
        FixedExecutor<0, 9> executor_;

      public:
        SNExecutor() : executor_{'@', 'S', 'N', '\r', '\n'} {}

        template <typename Serial>
        nb::Poll<SerialNumber> poll(Serial &serial) {
            auto &body = POLL_UNWRAP_OR_RETURN(executor_.poll(serial)).get();
            return SerialNumber{body};
        }
    };
} // namespace media::uhf
