#pragma once

#include "./executor.h"
#include "./initialization.h"
#include <nb/poll.h>

namespace net::link::uhf {
    class UHFFacade {
        UHFExecutor executor_;
        etl::optional<Initializer> initializer_;

        inline nb::Poll<void> wait_for_initialization() {
            return initializer_.has_value() ? nb::pending : nb::ready();
        }

      public:
        UHFFacade(nb::stream::ReadableWritableStream &stream)
            : executor_{stream},
              initializer_{Initializer{executor_}} {}

        inline bool is_supported_address_type(AddressType type) const {
            return executor_.is_supported_address_type(type);
        }

        inline nb::Poll<FrameTransmissionFuture>
        send_data(const Address &address, const frame::BodyLength length) {
            POLL_UNWRAP_OR_RETURN(wait_for_initialization());
            return executor_.send_data(address, length);
        }

        inline nb::Poll<FrameReceptionFuture> execute(util::Time &time, util::Rand &rand) {
            if (initializer_.has_value()) {
                POLL_UNWRAP_OR_RETURN(initializer_.value().execute(time, rand));
                initializer_ = etl::nullopt;
            }

            return executor_.execute(time, rand);
        }
    };
} // namespace net::link::uhf
