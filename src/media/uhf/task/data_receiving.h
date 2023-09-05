#pragma once

#include "../command/dr.h"
#include <nb/lock.h>

namespace media::uhf {
    template <typename Serial>
    class DataReceivingTask {
        nb::lock::Guard<memory::Owned<Serial>> serial_;

        DRExecutor<Serial> executor_;

      public:
        inline DataReceivingTask(
            nb::lock::Guard<memory::Owned<Serial>> &&serial,
            nb::Promise<ResponseReader<Serial>> &&body
        )
            : serial_{etl::move(serial)},
              executor_{etl::move(body)} {}

        inline nb::Poll<nb::Empty> poll() {
            return executor_.poll(serial_.get());
        }
    };
} // namespace media::uhf
