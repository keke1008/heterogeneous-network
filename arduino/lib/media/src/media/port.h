#pragma once

#include "./detector.h"
#include "./ethernet.h"
#include "./serial.h"
#include "./uhf.h"
#include "./wifi.h"
#include <etl/expected.h>
#include <etl/variant.h>
#include <util/visitor.h>

namespace media {
    struct SerialMediaPortTag {};

    struct EthernetMediaPortTag {};

    inline constexpr EthernetMediaPortTag ethernet_media_port_tag{};
    inline constexpr SerialMediaPortTag serial_media_port_tag{};

    template <nb::AsyncReadableWritable RW>
    class MediaPort {
        net::link::FrameBroker broker_;
        etl::variant<
            MediaDetector<RW>,
            uhf::UhfInteractor<RW>,
            wifi::WifiInteractor<RW>,
            serial::SerialInteractor<RW>,
            ethernet::EthernetInteractor>
            state_;

      public:
        MediaPort() = delete;
        MediaPort(const MediaPort &) = delete;
        MediaPort(MediaPort &&) = delete;
        MediaPort &operator=(const MediaPort &) = delete;
        MediaPort &operator=(MediaPort &&) = delete;

        MediaPort(
            SerialMediaPortTag,
            memory::Static<RW> &serial,
            memory::Static<net::link::LinkFrameQueue> &queue,
            net::link::MediaPortNumber port,
            util::Time &time
        )
            : broker_{queue, port},
              state_{MediaDetector<RW>{serial, time}} {}

        MediaPort(
            EthernetMediaPortTag,
            memory::Static<net::link::LinkFrameQueue> &queue,
            net::link::MediaPortNumber port,
            util::Time &time,
            util::Rand &rand
        )
            : broker_{queue, port},
              state_{etl::in_place_type<ethernet::EthernetInteractor>, broker_, time, rand} {}

        inline etl::optional<net::link::Address> broadcast_address(net::link::AddressType type
        ) const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return etl::nullopt; },
                    [](const auto &media) { return media.broadcast_address(); },
                },
                state_
            );
        }

        inline constexpr net::link::AddressTypeSet supported_address_types() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return net::link::AddressTypeSet{}; },
                    [](const auto &media) { return media.supported_address_types(); },
                },
                state_
            );
        }

        inline net::link::MediaInfo get_media_info() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return net::link::MediaInfo{}; },
                    [](const auto &media) { return media.get_media_info(); },
                },
                state_
            );
        }

      private:
        void try_replace_interactor(util::Time &time) {
            MediaDetector<RW> &detector = etl::get<MediaDetector<RW>>(state_);
            auto poll_media_type = detector.poll(time);
            if (poll_media_type.is_pending()) {
                return;
            }
            auto &stream = *detector.stream();

            switch (poll_media_type.unwrap()) {
            case net::link::MediaType::UHF: {
                state_.template emplace<uhf::UhfInteractor<RW>>(stream, etl::move(broker_));
                break;
            }
            case net::link::MediaType::Wifi: {
                state_.template emplace<wifi::WifiInteractor<RW>>(stream, etl::move(broker_), time);
                break;
            }
            case net::link::MediaType::Serial: {
                state_.template emplace<serial::SerialInteractor<RW>>(stream, etl::move(broker_));
                break;
            }
            }
        }

      public:
        void execute(net::frame::FrameService &fs, util::Time &time, util::Rand &rand) {
            etl::visit(
                util::Visitor{
                    [&](MediaDetector<RW> &) { try_replace_interactor(time); },
                    [&](uhf::UhfInteractor<RW> &media) { media.execute(fs, time, rand); },
                    [&](wifi::WifiInteractor<RW> &media) { media.execute(fs, time); },
                    [&](serial::SerialInteractor<RW> &media) { media.execute(fs, time); },
                    [&](ethernet::EthernetInteractor &media) { media.execute(fs, time); },
                },
                state_
            );
        }

        inline net::link::MediaPortOperationResult
        serial_try_initialize_local_address(const net::link::Address &address) {
            if (!etl::holds_alternative<serial::SerialInteractor<RW>>(state_)) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            auto &media = etl::get<serial::SerialInteractor<RW>>(state_);
            return media.try_initialize_local_address(address)
                ? net::link::MediaPortOperationResult::Success
                : net::link::MediaPortOperationResult::Failure;
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>
        wifi_join_ap(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password,
            util::Time &time
        ) {
            if (!etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            wifi::WifiInteractor<RW> &media = etl::get<wifi::WifiInteractor<RW>>(state_);
            return media.join_ap(ssid, password, time);
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>
        wifi_start_server(uint16_t port, util::Time &time) {
            if (!etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            wifi::WifiInteractor<RW> &media = etl::get<wifi::WifiInteractor<RW>>(state_);
            return media.close_server(time);
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>
        wifi_close_server(util::Time &time) {
            if (!etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            wifi::WifiInteractor<RW> &media = etl::get<wifi::WifiInteractor<RW>>(state_);
            return media.close_server(time);
        }

        inline net::link::MediaPortOperationResult
        ethernet_set_local_ip_address(etl::span<const uint8_t> ip) {
            if (!etl::holds_alternative<ethernet::EthernetInteractor>(state_)) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            ethernet::EthernetInteractor &media = etl::get<ethernet::EthernetInteractor>(state_);
            return media.set_local_ip_address(ip);
        }

        inline net::link::MediaPortOperationResult
        ethernet_set_subnet_mask(etl::span<const uint8_t> mask) {
            if (!etl::holds_alternative<ethernet::EthernetInteractor>(state_)) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            ethernet::EthernetInteractor &media = etl::get<ethernet::EthernetInteractor>(state_);
            return media.set_subnet_mask(mask);
        }
    };
} // namespace media
