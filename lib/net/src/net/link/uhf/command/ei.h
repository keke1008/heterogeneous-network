#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace net::link::uhf {
    class EIExecutor {
        FixedExecutor<2, 2> executor_;

      public:
        EIExecutor(ModemId equipment_id)
            : executor_{'@', 'E', 'I', equipment_id.span(), '\r', '\n'} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
