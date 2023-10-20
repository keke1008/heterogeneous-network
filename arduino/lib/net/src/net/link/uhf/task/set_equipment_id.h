#pragma once

#include "../command/ei.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class SetEquipmentIdTask {
        EIExecutor executor_;
        nb::Promise<void> promise_;

      public:
        inline SetEquipmentIdTask(ModemId equipment_id, nb::Promise<void> &&promise)
            : executor_{equipment_id},
              promise_{etl::move(promise)} {}

        inline nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            promise_.set_value();
            return nb::ready();
        }
    };
} // namespace net::link::uhf
