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

    class MediaPort {
        etl::variant<
            MediaDetector,
            uhf::UhfInteractor,
            wifi::WifiInteractor,
            serial::SerialInteractor>
            state_;
        FrameBroker broker_;

      public:
        MediaPort() = delete;
        MediaPort(const MediaPort &) = delete;
        MediaPort(MediaPort &&) = delete;
        MediaPort &operator=(const MediaPort &) = delete;
        MediaPort &operator=(MediaPort &&) = delete;

        MediaPort(
            nb::stream::ReadableWritableStream &serial,
            util::Time &time,
            FrameBroker &&broker
        )
            : state_{MediaDetector{serial, time}},
              broker_{etl::move(broker)} {}

        inline nb::Poll<bool> is_unicast_supported(AddressType type) const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector) -> nb::Poll<bool> { return nb::pending; },
                    [type](const auto &media) { return media.is_unicast_supported(type); },
                },
                state_
            );
        }

        inline nb::Poll<bool> is_broadcast_supported(AddressType type) const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector) -> nb::Poll<bool> { return nb::pending; },
                    [type](const auto &media) { return media.is_broadcast_supported(type); },
                },
                state_
            );
        }

        inline MediaInfo get_media_info() const {
            return etl::visit(
                util::Visitor{
                    [](const MediaDetector) { return MediaInfo{}; },
                    [](const auto &media) { return media.get_media_info(); },
                },
                state_
            );
        }

      private:
        void try_replace_interactor(util::Time &time) {
            auto &detector = etl::get<MediaDetector>(state_);
            auto poll_media_type = detector.poll(time);
            if (poll_media_type.is_pending()) {
                return;
            }
            auto &stream = detector.stream();

            switch (poll_media_type.unwrap()) {
            case MediaType::UHF: {
                state_ = uhf::UhfInteractor{stream, etl::move(broker_)};
                break;
            }
            case MediaType::Wifi: {
                state_ = wifi::WifiInteractor{stream, etl::move(broker_)};
                break;
            }
            case MediaType::Serial: {
                state_ = serial::SerialInteractor{stream, etl::move(broker_)};
                break;
            }
            }
        }

      public:
        void execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            etl::visit(
                util::Visitor{
                    [&](MediaDetector &) { try_replace_interactor(time); },
                    [&](uhf::UhfInteractor &media) { media.execute(frame_service, time, rand); },
                    [&](auto &media) { media.execute(frame_service); },
                },
                state_
            );
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            if (etl::holds_alternative<wifi::WifiInteractor>(state_)) {
                return etl::get<wifi::WifiInteractor>(state_).join_ap(ssid, password);
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>
        start_wifi_server(uint16_t port) {
            if (etl::holds_alternative<wifi::WifiInteractor>(state_)) {
                return etl::get<wifi::WifiInteractor>(state_).start_server(port);
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }

        etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError> close_wifi_server() {
            if (etl::holds_alternative<wifi::WifiInteractor>(state_)) {
                return etl::get<wifi::WifiInteractor>(state_).close_server();
            }
            return etl::expected<nb::Poll<nb::Future<bool>>, UnsupportedOperationError>{
                etl::unexpect,
            };
        }
    };
} // namespace net::link
