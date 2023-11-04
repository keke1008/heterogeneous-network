#pragma once

#include "./frame.h"
#include "./receiver.h"
#include "./sender.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SerialInteractor {
        nb::stream::ReadableWritableStream &stream_;

        FrameSender sender_;
        FrameReceiver receiver_;

      public:
        SerialInteractor() = delete;
        SerialInteractor(const SerialInteractor &) = delete;
        SerialInteractor(SerialInteractor &&) = default;
        SerialInteractor &operator=(const SerialInteractor &) = delete;
        SerialInteractor &operator=(SerialInteractor &&) = delete;

        explicit SerialInteractor(
            const FrameBroker &broker,
            nb::stream::ReadableWritableStream &stream
        )
            : stream_{stream},
              sender_{broker},
              receiver_{broker} {}

      public:
        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::Serial;
        }

        inline nb::Poll<Address> get_address() const {
            const auto &address = receiver_.get_address();
            return address ? nb::ready(Address{*address}) : nb::pending;
        }

        inline void execute(frame::FrameService &service) {
            auto address = receiver_.execute(service, stream_);
            if (address) {
                sender_.set_self_address_if_not_set(*address);
            }

            sender_.execute(stream_);
        }
    };
} // namespace net::link::serial
