#pragma once

#include "./port.h"

namespace media {
    template <nb::AsyncReadableWritable RW>
    class MediaService {
        etl::vector<MediaPortRef<RW>, net::link::MAX_MEDIA_PER_NODE> ports_;

      public:
        using MediaPortType = MediaPortRef<RW>;

        void register_port(memory::Static<SerialPortMediaPort<RW>> &serial) {
            FASSERT(!ports_.full());
            uint8_t next = ports_.size();
            serial->initialize_media_port(net::link::MediaPortNumber{next});
            ports_.emplace_back(serial);
        }

        void register_port(memory::Static<EthernetShieldMediaPort> &ethernet) {
            FASSERT(!ports_.full());
            uint8_t next = ports_.size();
            ethernet->initialize_media_port(net::link::MediaPortNumber{next});
            ports_.emplace_back(ethernet);
        }

        inline net::link::AddressTypeSet supported_address_types() const {
            net::link::AddressTypeSet set;
            for (const auto &port : ports_) {
                set |= port->supported_address_types();
            }
            return set;
        }

        inline etl::optional<etl::reference_wrapper<MediaPortType>>
        get_media_port(net::link::MediaPortNumber port) {
            uint8_t index = port.value();
            return index < ports_.size() ? etl::optional(etl::ref(ports_[index])) : etl::nullopt;
        }

        etl::optional<net::link::Address> get_media_address() const {
            for (const MediaPortType &port : ports_) {
                const auto &info = port.get_media_info();
                if (info.address.has_value()) {
                    return info.address;
                }
            }

            return etl::nullopt;
        }

        void get_media_addresses(
            etl::vector<net::link::Address, net::link::MAX_MEDIA_PER_NODE> &addresses
        ) const {
            for (const MediaPortType &port : ports_) {
                const auto &info = port.get_media_info();
                if (info.address.has_value()) {
                    addresses.push_back(info.address.value());
                }
            }
        }

        void get_media_info(
            etl::array<etl::optional<net::link::MediaInfo>, net::link::MAX_MEDIA_PER_NODE>
                &media_info
        ) const {
            for (uint8_t i = 0; i < media_info.size(); i++) {
                const auto &info = ports_[i].get_media_info();
                if (info.address.has_value()) {
                    media_info[i] = info;
                }
            }
        }

        inline etl::optional<net::link::Address> get_broadcast_address(net::link::AddressType type
        ) const {
            for (const MediaPortType &port : ports_) {
                if (port.supported_address_types().test(type)) {
                    return port.broadcast_address(type);
                }
            }
            return etl::nullopt;
        }

        inline void execute(net::frame::FrameService &fs, util::Time &time, util::Rand &rand) {
            for (MediaPortType &port : ports_) {
                port.execute(fs, time, rand);
            }
        }
    };
} // namespace media
