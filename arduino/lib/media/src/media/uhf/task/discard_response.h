#pragma once

#include "../response.h"
#include "./interruption.h"
#include <nb/serde.h>

namespace media::uhf {
    template <nb::AsyncReadable R>
    class DiscardResponseTask {
        nb::LockGuard<etl::reference_wrapper<R>> readable_;
        nb::de::AsyncDiscardingSingleLineDeserializer deserializer_;

      public:
        explicit DiscardResponseTask(nb::LockGuard<etl::reference_wrapper<R>> &&readable)
            : readable_{etl::move(readable)} {}

        nb::Poll<void> execute() {
            POLL_UNWRAP_OR_RETURN(deserializer_.deserialize(readable_->get()));
            return nb::ready();
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<R> &&_) {
            return UhfHandleResponseResult::Invalid;
        }

        inline TaskInterruptionResult interrupt() {
            return TaskInterruptionResult::Aborted;
        }
    };
} // namespace media::uhf
