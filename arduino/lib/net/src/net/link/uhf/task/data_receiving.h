#pragma once

#include "../command/dr.h"

namespace net::link::uhf {
    class DataReceivingTask {
        DRExecutor executor_;

      public:
        DataReceivingTask() = default;
        DataReceivingTask(const DataReceivingTask &) = delete;
        DataReceivingTask(DataReceivingTask &&) = default;
        DataReceivingTask &operator=(const DataReceivingTask &) = delete;
        DataReceivingTask &operator=(DataReceivingTask &&) = default;

        inline nb::Poll<UhfFrame>
        poll(frame::FrameService &service, nb::stream::ReadableWritableStream &stream) {
            return executor_.poll(service, stream);
        }
    };
} // namespace net::link::uhf
