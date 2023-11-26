#pragma once

#include "./detector.h"
#include "./serial.h"
#include "./uhf.h"
#include "./wifi.h"
#include <etl/expected.h>
#include <etl/variant.h>
#include <util/visitor.h>

namespace net::link {
    struct UnsupportedOperationError {};

    template <nb::AsyncReadableWritable RW>
    class MediaPort {
        etl::variant<
            MediaDetector<RW>,
            uhf::UhfInteractor<RW>,
            wifi::WifiInteractor<RW>,
            serial::SerialInteractor<RW>>
            state_;
        FrameBroker broker_;

      public:
        MediaPort() = delete;
        MediaPort(const MediaPort &) = delete;
        MediaPort(MediaPort &&) = delete;
        MediaPort &operator=(const MediaPort &) = delete;
        MediaPort &operator=(MediaPort &&) = delete;

        MediaPort(
            memory::Static<RW> &serial,
            util::Time &time,
            memory::Static<LinkFrameQueue> &queue
        )
            : state_{MediaDetector<RW>{serial, time}},
              broker_{FrameBroker{queue}} {}

        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return AddressTypeSet{}; },
                    [](const auto &media) { return media.unicast_supported_address_types(); },
                },
                state_
            );
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return AddressTypeSet{}; },
                    [](const auto &media) { return media.broadcast_supported_address_types(); },
                },
                state_
            );
        }

        inline MediaInfo get_media_info() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector<RW>) { return MediaInfo{}; },
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
            case MediaType::UHF: {
                state_.template emplace<uhf::UhfInteractor<RW>>(stream, etl::move(broker_));
                break;
            }
            case MediaType::Wifi: {
                state_.template emplace<wifi::WifiInteractor<RW>>(stream, etl::move(broker_));
                break;
            }
            case MediaType::Serial: {
                state_.template emplace<serial::SerialInteractor<RW>>(stream, etl::move(broker_));
                break;
            }
            }
        }

      public:
        void execute(frame::FrameService &fs, util::Time &time, util::Rand &rand) {
            etl::visit(
                util::Visitor{
                    [&](MediaDetector<RW> &) { try_replace_interactor(time); },
                    [&](uhf::UhfInteractor<RW> &media) { media.execute(fs, time, rand); },
                    [&](auto &media) { media.execute(fs); },
                },
                state_
            );
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            if (etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::get<wifi::WifiInteractor<RW>>(state_).join_ap(ssid, password);
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>
        start_wifi_server(uint16_t port) {
            if (etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::get<wifi::WifiInteractor<RW>>(state_).start_server(port);
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError> close_wifi_server() {
            if (etl::holds_alternative<wifi::WifiInteractor<RW>>(state_)) {
                return etl::get<wifi::WifiInteractor<RW>>(state_).close_server();
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }
    };
} // namespace net::link
