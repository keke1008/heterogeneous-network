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

        template <nb::AsyncReadable R>
        inline nb::Poll<UhfFrame> poll(frame::FrameService &service, R &r) {
            return executor_.poll(service, r);
        }
    };
} // namespace net::link::uhf
