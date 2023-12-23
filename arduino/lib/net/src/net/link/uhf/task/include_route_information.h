#pragma once

#include "./fixed.h"

namespace net::link::uhf {
    template <nb::AsyncReadableWritable RW>
    class IncludeRouteInformationTask {
        FixedTask<RW, nb::ser::AsyncStaticSpanSerializer, UhfResponseType::RI, 2> task_{"@RION\r\n"
        };

      public:
        inline nb::Poll<void> execute(nb::Lock<etl::reference_wrapper<RW>> &rw) {
            POLL_UNWRAP_OR_RETURN(task_.execute(rw));
            return nb::ready();
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            return task_.handle_response(etl::move(res));
        }
    };
} // namespace net::link::uhf
