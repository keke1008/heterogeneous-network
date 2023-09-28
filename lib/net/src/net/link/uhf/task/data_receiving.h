#pragma once

#include "../command/dr.h"

namespace net::link::uhf {
    class DataReceivingTask {
        DRExecutor executor_;

      public:
        inline DataReceivingTask() = default;

        inline nb::Poll<void> poll(
            net::frame::FrameService<Address> &service,
            nb::stream::ReadableWritableStream &stream
        ) {
            return executor_.poll(service, stream);
        }
    };
} // namespace net::link::uhf
