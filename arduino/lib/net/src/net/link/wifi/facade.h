#pragma once

#include "./executor.h"
#include <net/frame/service.h>

namespace net::link::wifi {
    class WifiFacade {
        WifiExecutor executor_;
        nb::Future<bool> initialization_barrier_;

      public:
        explicit inline WifiFacade(nb::stream::ReadableWritableStream &stream)
            : executor_{stream},
              initialization_barrier_{etl::move(executor_.initialize().unwrap())} {}

        inline bool is_supported_address_type(AddressType type) const {
            return executor_.is_supported_address_type(type);
        }

        inline etl::optional<Address> get_address() const {
            return executor_.get_address();
        }

        inline nb::Poll<void> send_frame(SendingFrame &frame) {
            return executor_.send_frame(frame);
        }

        inline nb::Poll<Frame> receive_frame(frame::ProtocolNumber protocol_number) {
            return executor_.receive_frame(protocol_number);
        }

        template <net::frame::IFrameService FrameService>
        inline void execute(FrameService &service) {
            return executor_.execute(service);
        }

        inline nb::Poll<nb::Future<bool>>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            return executor_.join_ap(ssid, password);
        }
    };
} // namespace net::link::wifi
