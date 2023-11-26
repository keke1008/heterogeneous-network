#pragma once

#include "../command/ri.h"

namespace net::link::uhf {
    class IncludeRouteInformationTask {
        RIExecutor ri_executor_;

      public:
        IncludeRouteInformationTask() : ri_executor_{} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> poll(RW &stream) {
            POLL_UNWRAP_OR_RETURN(ri_executor_.poll(stream));
            return nb::ready();
        }
    };
} // namespace net::link::uhf
