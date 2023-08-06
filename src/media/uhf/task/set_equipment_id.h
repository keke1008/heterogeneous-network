#pragma once

#include "../command/ei.h"
#include <nb/lock.h>

namespace media::uhf {
    template <typename Serial>
    class SetEquipmentIdTask {
        nb::lock::Guard<memory::Owned<Serial>> serial_;

        EIExecutor executor_;

      public:
        inline SetEquipmentIdTask(nb::lock::Guard<Serial> &&serial, ModemId equipment_id)
            : serial_{etl::move(serial)},
              executor_{equipment_id} {}

        inline nb::Poll<nb::Empty> poll() {
            return executor_.poll(serial_.get());
        }
    };
} // namespace media::uhf
