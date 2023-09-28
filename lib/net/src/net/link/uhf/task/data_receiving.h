#pragma once

#include "../command/dr.h"

namespace net::link::uhf {
    class DataReceivingTask {
        DRExecutor executor_;

      public:
        inline DataReceivingTask() = default;

        template <net::frame::IFrameService<Address> FrameService>
        inline nb::Poll<void>
        poll(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            return executor_.poll(service, stream);
        }
    };
} // namespace net::link::uhf
