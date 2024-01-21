#pragma once

#include "./receiver.h"
#include "./sender.h"
#include <etl/optional.h>
#include <net/frame.h>

namespace media::serial {
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

        explicit SerialInteractor(RW &rw, memory::Static<net::link::FrameBroker> &broker)
            : rw_{rw},
              sender_{broker},
              receiver_{broker} {}

      public:
        inline constexpr net::link::AddressTypeSet supported_address_types() const {
            return net::link::AddressTypeSet{net::link::AddressType::Serial};
        }

        inline etl::optional<net::link::Address> broadcast_address() const {
            return etl::nullopt;
        }

        inline net::link::MediaInfo get_media_info() const {
            const auto &address = receiver_.get_self_address();
            return net::link::MediaInfo{
                .address_type = net::link::AddressType::Serial,
                .address = address ? etl::optional(net::link::Address{*address}) : etl::nullopt,
            };
        }

        inline void execute(net::frame::FrameService &service, util::Time &time) {
            receiver_.execute(service, rw_, time);
            auto self_address = receiver_.get_self_address();
            if (self_address.has_value()) {
                sender_.execute(rw_, *self_address, receiver_.get_remote_address());
            }
        }

        inline bool try_initialize_local_address(net::link::Address address) {
            if (address.type() != net::link::AddressType::Serial) {
                return false;
            }
            return receiver_.try_initialize_local_address(SerialAddress(address));
        }
    };
} // namespace media::serial
