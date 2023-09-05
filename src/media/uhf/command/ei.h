#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    class EIExecutor {
        FixedExecutor<2, 2> executor_;

      public:
        EIExecutor(ModemId equipment_id)
            : executor_{'@', 'E', 'I', equipment_id.get<0>(), equipment_id.get<1>(), '\r', '\n'} {}

        template <typename Serial>
        nb::Poll<nb::Empty> poll(Serial &serial) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(serial)).get();
            return nb::Ready{nb::Empty{}};
        }
    };
} // namespace media::uhf
