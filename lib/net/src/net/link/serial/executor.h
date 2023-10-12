#pragma once

#include "./address.h"
#include "./receiver.h"
#include "./sender.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SerialExecutor {
        nb::stream::ReadableWritableStream &stream_;

        SerialAddress address_;
        FrameSender sender_;
        FrameReceiver receiver_;

      public:
        SerialExecutor() = delete;
        SerialExecutor(const SerialExecutor &) = delete;
        SerialExecutor(SerialExecutor &&) = default;
        SerialExecutor &operator=(const SerialExecutor &) = delete;
        SerialExecutor &operator=(SerialExecutor &&) = delete;

        explicit SerialExecutor(nb::stream::ReadableWritableStream &stream, SerialAddress address)
            : stream_{stream},
              address_{address},
              sender_{address},
              receiver_{address} {}

      public:
        inline void set_address(SerialAddress address) {
            address_ = address;
        }

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::Serial;
        }

        inline etl::optional<Address> get_address() const {
            return Address{address_};
        }

        inline nb::Poll<void> send_frame(Frame &&frame) {
            return sender_.send_frame(etl::move(frame));
        }

        inline nb::Poll<Frame> receive_frame() {
            return receiver_.receive_frame();
        }

        template <net::frame::IFrameService Service>
        void execute(Service &service) {
            receiver_.execute(service, stream_);
            sender_.execute(stream_);
        }
    };
} // namespace net::link::serial
