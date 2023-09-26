#pragma once

#include "./executor.h"

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

        inline nb::Poll<FrameTransmission>
        send_data(const Address &destination, frame::BodyLength body_length) {
            return executor_.send_data(destination, body_length);
        }

        inline nb::Poll<FrameReception> execute() {
            return executor_.execute();
        }
    };
} // namespace net::link::wifi
