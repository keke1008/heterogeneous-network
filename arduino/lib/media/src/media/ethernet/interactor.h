#pragma once

#include "./receiver.h"
#include "./sender.h"
#include "./shield.h"
#include <net.h>
#include <util/rand.h>

namespace media::ethernet {
    class EthernetInteractor {
        EthernetShield shield_;
        FrameSender sender_;
        FrameReceiver receiver_;

      public:
        EthernetInteractor() = delete;
        EthernetInteractor(const EthernetInteractor &) = delete;
        EthernetInteractor(EthernetInteractor &&) = delete;
        EthernetInteractor &operator=(const EthernetInteractor &) = delete;
        EthernetInteractor &operator=(EthernetInteractor &&) = delete;

        explicit EthernetInteractor(
            const net::link::FrameBroker &broker,
            util::Time &time,
            util::Rand &rand
        )
            : shield_{time, rand},
              sender_{broker},
              receiver_{broker} {}

        void execute(net::frame::FrameService &fs, util::Time &time) {
            auto link_state = shield_.execute(time);
            if (link_state == LinkState::Down) {
                return;
            } else if (link_state == LinkState::JustDown) {
                sender_.on_link_down();
                receiver_.on_link_down();
                return;
            } else { // link_state == LinkState::Up
                sender_.execute(udp);
                receiver_.execute(fs, udp, time);
            }
        }

        inline etl::optional<net::link::Address> broadcast_address() const {
            return etl::nullopt;
        }

        inline constexpr net::link::AddressTypeSet supported_address_types() const {
            return net::link::AddressTypeSet{net::link::AddressType::Udp};
        }

        inline net::link::MediaInfo get_media_info() const {
            const auto &opt_addr = shield_.get_local_address();
            return net::link::MediaInfo{
                .address_type = net::link::AddressType::Udp,
                .address = opt_addr ? etl::optional(net::link::Address(*opt_addr)) : etl::nullopt,
            };
        }

        inline net::link::MediaPortOperationResult
        set_local_ip_address(etl::span<const uint8_t> address) {
            return shield_.set_local_ip_address(address);
        }

        inline net::link::MediaPortOperationResult set_subnet_mask(etl::span<const uint8_t> mask) {
            return shield_.set_subnet_mask(mask);
        }
    };
} // namespace media::ethernet
