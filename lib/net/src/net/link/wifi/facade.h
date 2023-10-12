#pragma once

#include "./executor.h"
#include <net/frame/service.h>

namespace net::link::wifi {
    class WifiFacade {
        WifiExecutor executor_;
        nb::Future<bool> initialization_barrier_;

      public:
        explicit inline WifiFacade(nb::stream::ReadableWritableStream &stream, uint16_t port_number)
            : executor_{stream, port_number},
              initialization_barrier_{etl::move(executor_.initialize().unwrap())} {}

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
            return executor_.execute(service);
        }
    };
} // namespace net::link::wifi
