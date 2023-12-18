#pragma once

#include "./receiver.h"
#include "./sender.h"
#include <etl/optional.h>
#include <net/frame.h>

namespace net::link::serial {
    template <nb::AsyncReadableWritable RW>
    class SerialInteractor {
        RW &rw_;

        FrameSender sender_;
        FrameReceiver receiver_;

      public:
        SerialInteractor() = delete;
        SerialInteractor(const SerialInteractor &) = delete;
        SerialInteractor(SerialInteractor &&) = default;
        SerialInteractor &operator=(const SerialInteractor &) = delete;
        SerialInteractor &operator=(SerialInteractor &&) = delete;

        explicit SerialInteractor(RW &rw, const FrameBroker &broker)
            : rw_{rw},
              sender_{broker},
              receiver_{broker} {}

      public:
        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            return AddressTypeSet{AddressType::Serial};
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            return AddressTypeSet{};
        }

        inline MediaInfo get_media_info() const {
            const auto &address = receiver_.get_self_address();
            return MediaInfo{
                .address_type = AddressType::Serial,
                .address = address ? etl::optional(Address{*address}) : etl::nullopt,
            };
        }

        inline void execute(frame::FrameService &service) {
            receiver_.execute(service, rw_);
            auto self_address = receiver_.get_self_address();
            if (self_address.has_value()) {
                sender_.execute(rw_, *self_address);
            }
        }

        inline bool try_initialize_local_address(SerialAddress address) {
            receiver_.try_initialize_local_address(address);
        }
    };
} // namespace net::link::serial
