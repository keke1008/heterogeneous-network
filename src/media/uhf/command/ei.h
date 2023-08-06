#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace media::uhf {
    template <typename Serial>
    class EIExecutor {
        FixedExecutor<Serial, 2, 2> executor_;

      public:
        EIExecutor(Serial &&serial, common::ModemId equipment_id)
            : executor_{etl::move(serial),     '@',  'E', 'I', equipment_id.get<0>(),
                        equipment_id.get<1>(), '\r', '\n'} {}

        nb::Poll<nb::Empty> poll() {
            POLL_UNWRAP_OR_RETURN(executor_.poll()).get();
            return nb::Ready{nb::Empty{}};
        }
    };
} // namespace media::uhf
