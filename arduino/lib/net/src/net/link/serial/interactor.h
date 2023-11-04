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
            nb::stream::ReadableWritableStream &stream,
            const FrameBroker &broker
        )
            : stream_{stream},
              sender_{broker},
              receiver_{broker} {}

      public:
        inline bool is_unicast_supported(AddressType type) const {
            return type == AddressType::Serial;
        }

        inline bool is_broadcast_supported(AddressType type) const {
            return false;
        }

        inline MediaInfo get_media_info() const {
            const auto &address = receiver_.get_address();
            return MediaInfo{
                .address_type = AddressType::Serial,
                .address = address ? etl::optional(Address{*address}) : etl::nullopt,
            };
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
