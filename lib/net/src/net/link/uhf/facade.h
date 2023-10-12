#pragma once

#include "./executor.h"
#include "./initialization.h"
#include <nb/poll.h>
#include <net/frame/service.h>

namespace net::link::uhf {
    class UHFFacade {
        UHFExecutor executor_;
        etl::optional<Initializer> initializer_;
        util::Time &time_;
        util::Rand &rand_;

        inline nb::Poll<void> wait_for_initialization() {
            return initializer_.has_value() ? nb::pending : nb::ready();
        }

      public:
        UHFFacade(nb::stream::ReadableWritableStream &stream, util::Time &time, util::Rand &rand)
            : executor_{stream},
              initializer_{Initializer{executor_}},
              time_{time},
              rand_{rand} {}

        inline bool is_supported_address_type(AddressType type) const {
            return executor_.is_supported_address_type(type);
        }

        inline nb::Poll<void> send_frame(Frame &&frame) {
            return executor_.send_frame(etl::move(frame));
        }

        inline nb::Poll<Frame> receive_frame() {
            return executor_.receive_frame();
        }

        template <net::frame::IFrameService FrameService>
        inline void execute(FrameService &service) {
            if (initializer_.has_value()) {
                (initializer_.value().execute(service, time_, rand_));
                initializer_ = etl::nullopt;
            }

            return executor_.execute(service, time_, rand_);
        }
    };
} // namespace net::link::uhf
