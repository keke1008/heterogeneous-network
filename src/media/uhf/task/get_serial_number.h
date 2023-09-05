#pragma once

#include "../command/sn.h"
#include <nb/lock.h>

namespace media::uhf {
    template <typename Serial>
    class GetSerialNumberTask {
        nb::lock::Guard<memory::Owned<Serial>> serial_;

        SNExecutor executor;

      public:
        inline GetSerialNumberTask(nb::lock::Guard<memory::Owned<Serial>> &&serial)
            : serial_{serial} {}

        inline nb::Poll<SerialNumber> poll() {
            return executor.poll(serial_.get());
        }
    };
} // namespace media::uhf
