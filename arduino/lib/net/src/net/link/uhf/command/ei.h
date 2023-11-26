#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/serde.h>

namespace net::link::uhf {
    class EIExecutor {
        AsyncFixedExecutor<AsyncModemIdSerializer, 2> executor_;

      public:
        EIExecutor(ModemId equipment_id)
            : executor_{'E', 'I', AsyncModemIdSerializer{equipment_id}} {}

        template <nb::de::AsyncReadable R>
        nb::Poll<void> poll(R &r) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(r));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
