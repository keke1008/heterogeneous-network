#pragma once

#include "./executor.h"
#include "./initialization.h"
#include <nb/poll.h>
#include <net/frame/service.h>

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

        template <net::frame::IFrameService FrameService>
        inline void execute(FrameService &service, util::Time &time, util::Rand &rand) {
            if (initializer_.has_value()) {
                (initializer_.value().execute(service, time, rand));
                initializer_ = etl::nullopt;
            }

            return executor_.execute(service, time, rand);
        }
    };
} // namespace net::link::uhf
