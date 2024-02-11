#pragma once

#include <memory/lifetime.h>
#include <nb/lock.h>
#include <nb/serde.h>

namespace media::wifi {
    struct IsLastCRTag {};

    inline constexpr IsLastCRTag is_last_cr{};

    template <nb::AsyncReadable R>
    class UnknownMessageHandler {
        nb::de::AsyncDiscardingSingleLineDeserializer discarder_;
        nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> readable_;

      public:
        UnknownMessageHandler() = delete;

        explicit UnknownMessageHandler(nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> &&r)
            : discarder_{},
              readable_{etl::move(r)} {}

        explicit UnknownMessageHandler(
            IsLastCRTag,
            nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> &&r
        )
            : discarder_{'\r'},
              readable_{etl::move(r)} {}

        inline nb::Poll<void> execute() {
            POLL_UNWRAP_OR_RETURN(discarder_.deserialize(*readable_->get()));
            return nb::ready();
        }
    };
} // namespace media::wifi
