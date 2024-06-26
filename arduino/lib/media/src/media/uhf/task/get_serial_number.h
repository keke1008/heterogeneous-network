#pragma once

#include "../common.h"
#include "./fixed.h"
#include "./interruption.h"
#include <nb/future.h>

namespace media::uhf {
    template <nb::AsyncReadableWritable RW>
    class GetSerialNumberTask {
        static constexpr char COMMAND[] = "@SN\r\n";
        using Task = FixedTask<RW, nb::ser::AsyncStaticSpanSerializer, UhfResponseType::SN, 9>;

        Task task_{COMMAND};
        nb::Promise<etl::optional<SerialNumber>> promise_;

      public:
        inline GetSerialNumberTask(nb::Promise<etl::optional<SerialNumber>> &&promise)
            : promise_{etl::move(promise)} {}

        inline nb::Poll<void> execute(nb::Lock<etl::reference_wrapper<RW>> &rw) {
            auto result = POLL_UNWRAP_OR_RETURN(task_.execute(rw)) == FixedTaskResult::Ok
                ? etl::optional(SerialNumber{task_.span_result()})
                : etl::nullopt;
            promise_.set_value(result);
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
