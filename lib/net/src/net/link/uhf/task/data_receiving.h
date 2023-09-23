#pragma once

#include "../command/dr.h"

namespace net::link::uhf {
    class DataReceivingTask {
        DRExecutor executor_;

      public:
        inline DataReceivingTask(nb::Promise<DataReader> &&body) : executor_{etl::move(body)} {}

        inline nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            return executor_.poll(stream);
        }
    };
} // namespace net::link::uhf
