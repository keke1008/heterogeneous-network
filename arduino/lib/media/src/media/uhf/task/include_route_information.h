#pragma once

#include "./fixed.h"
#include "./interruption.h"

namespace media::uhf {
    template <nb::AsyncReadableWritable RW>
    class IncludeRouteInformationTask {
        static constexpr char COMMAND[] = "@RION\r\n";
        using Task = FixedTask<RW, nb::ser::AsyncStaticSpanSerializer, UhfResponseType::RI, 2>;

        Task task_{COMMAND};

      public:
        inline nb::Poll<void> execute(nb::Lock<etl::reference_wrapper<RW>> &rw) {
            POLL_UNWRAP_OR_RETURN(task_.execute(rw));
            return nb::ready();
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            return task_.handle_response(etl::move(res));
        }

        inline TaskInterruptionResult interrupt() {
            task_ = Task{COMMAND};
            return TaskInterruptionResult::Interrupted;
        }
    };
} // namespace media::uhf
