#pragma once

#include "../command/ei.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class SetEquipmentIdTask {
        EIExecutor executor_;

      public:
        inline SetEquipmentIdTask(ModemId equipment_id) : executor_{equipment_id} {}

        inline nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
