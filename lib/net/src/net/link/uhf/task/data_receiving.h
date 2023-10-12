#pragma once

#include "../command/dr.h"

namespace net::link::uhf {
    class DataReceivingTask {
        DRExecutor executor_;

      public:
        DataReceivingTask() = delete;
        DataReceivingTask(const DataReceivingTask &) = delete;
        DataReceivingTask(DataReceivingTask &&) = default;
        DataReceivingTask &operator=(const DataReceivingTask &) = delete;
        DataReceivingTask &operator=(DataReceivingTask &&) = default;

        explicit DataReceivingTask(bool discard_frame_) : executor_{discard_frame_} {}

        template <net::frame::IFrameService FrameService>
        inline nb::Poll<etl::optional<Frame>>
        poll(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            return executor_.poll(service, stream);
        }
    };
} // namespace net::link::uhf
