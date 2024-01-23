#pragma once

#include "./port/ethernet_shield.h"
#include "./port/serial_port.h"
#include <etl/expected.h>
#include <etl/variant.h>
#include <util/visitor.h>

namespace media {
    template <nb::AsyncReadableWritable RW>
    class MediaPortRef {
        etl::variant<
            etl::reference_wrapper<memory::Static<SerialPortMediaPort<RW>>>,
            etl::reference_wrapper<memory::Static<EthernetShieldMediaPort>>>
            ref_;

        inline MediaInteractorRef<RW, false> get_media_interactor_ref() {
            return etl::visit(
                util::Visitor{
                    [](etl::reference_wrapper<memory::Static<SerialPortMediaPort<RW>>> &media) {
                        return media.get()->get_media_interactor_ref();
                    },
                    [](etl::reference_wrapper<memory::Static<EthernetShieldMediaPort>> &media) {
                        return media.get()->get_media_interactor_ref<RW>();
                    },
                },
                ref_
            );
        }

        inline MediaInteractorRef<RW, true> get_media_interactor_ref() const {
            return etl::visit<MediaInteractorRef<RW, true>>(
                util::Visitor{
                    [](const etl::reference_wrapper<memory::Static<SerialPortMediaPort<RW>>> &p) {
                        return p.get()->get_media_interactor_cref();
                    },
                    [](const etl::reference_wrapper<memory::Static<EthernetShieldMediaPort>> &p) {
                        return p.get()->get_media_interactor_cref<RW>();
                    },
                },
                ref_
            );
        }

      public:
        MediaPortRef() = delete;
        MediaPortRef(const MediaPortRef &) = delete;
        MediaPortRef(MediaPortRef &&) = delete;
        MediaPortRef &operator=(const MediaPortRef &) = delete;
        MediaPortRef &operator=(MediaPortRef &&) = delete;

        MediaPortRef(memory::Static<SerialPortMediaPort<RW>> &serial) : ref_{etl::ref(serial)} {}

        MediaPortRef(memory::Static<EthernetShieldMediaPort> &ethernet)
            : ref_{etl::ref(ethernet)} {}

        inline etl::optional<net::link::Address> broadcast_address(net::link::AddressType type
        ) const {
            return get_media_interactor_ref().template visit<etl::optional<net::link::Address>>(
                util::Visitor{
                    [](const etl::monostate) { return etl::nullopt; },
                    [](const auto &media) { return media.broadcast_address(); },
                }
            );
        }

        inline constexpr net::link::AddressTypeSet supported_address_types() const {
            return get_media_interactor_ref().template visit<net::link::AddressTypeSet>(
                util::Visitor{
                    [](const etl::monostate) { return net::link::AddressTypeSet{}; },
                    [](const auto &media) { return media.supported_address_types(); },
                }
            );
        }

        inline net::link::MediaInfo get_media_info() const {
            return get_media_interactor_ref().template visit<net::link::MediaInfo>(util::Visitor{
                [](const etl::monostate) { return net::link::MediaInfo{}; },
                [](const auto &media) { return media.get_media_info(); },
            });
        }

      public:
        inline void execute(net::frame::FrameService &fs, util::Time &time, util::Rand &rand) {
            etl::visit(
                util::Visitor{
                    [&](etl::reference_wrapper<memory::Static<SerialPortMediaPort<RW>>> &serial) {
                        serial.get()->execute(fs, time, rand);
                    },
                    [&](etl::reference_wrapper<memory::Static<EthernetShieldMediaPort>> &ethernet) {
                        ethernet.get()->execute(fs, time);
                    },
                },
                ref_
            );
        }

        inline net::link::MediaPortOperationResult
        serial_try_initialize_local_address(const net::link::Address &address) {
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<serial::SerialInteractor<RW>>()) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            auto &media = interactor.template get<serial::SerialInteractor<RW>>();
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
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<wifi::WifiInteractor<RW>>()) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            auto &media = interactor.template get<wifi::WifiInteractor<RW>>();
            return media.join_ap(ssid, password, time);
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>
        wifi_start_server(uint16_t port, util::Time &time) {
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<wifi::WifiInteractor<RW>>()) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            auto &media = interactor.template get<wifi::WifiInteractor<RW>>();
            return media.start_server(port, time);
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>
        wifi_close_server(util::Time &time) {
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<wifi::WifiInteractor<RW>>()) {
                return etl::expected<
                    nb::Poll<nb::Future<bool>>, net::link::MediaPortUnsupportedOperation>{
                    etl::unexpected<net::link::MediaPortUnsupportedOperation>{
                        net::link::MediaPortUnsupportedOperation{}
                    }
                };
            }

            auto &media = interactor.template get<wifi::WifiInteractor<RW>>();
            return media.close_server(time);
        }

        inline net::link::MediaPortOperationResult
        ethernet_set_local_ip_address(etl::span<const uint8_t> ip) {
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<ethernet::EthernetInteractor>()) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            auto &media = interactor.template get<ethernet::EthernetInteractor>();
            return media.set_local_ip_address(ip);
        }

        inline net::link::MediaPortOperationResult
        ethernet_set_subnet_mask(etl::span<const uint8_t> mask) {
            auto &&interactor = get_media_interactor_ref();
            if (!interactor.template holds_alternative<ethernet::EthernetInteractor>()) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            auto &media = interactor.template get<ethernet::EthernetInteractor>();
            return media.set_subnet_mask(mask);
        }
    };
} // namespace media
