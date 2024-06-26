#pragma once

#include "./task.h"
#include "net/link/address.h"

namespace media::wifi {
    template <nb::AsyncReadable RW>
    class WifiInteractor {
        TaskExecutor<RW, RW> task_executor_;
        LocalServerState server_state_;

        etl::optional<nb::Future<bool>> initialization_result_;
        etl::optional<nb::Future<UdpIpAddress>> get_ip_result_;

      public:
        WifiInteractor() = delete;
        WifiInteractor(const WifiInteractor &) = delete;
        WifiInteractor(WifiInteractor &&) = delete;
        WifiInteractor &operator=(const WifiInteractor &) = delete;
        WifiInteractor &operator=(WifiInteractor &&) = delete;

        WifiInteractor(
            memory::Static<RW> &rw,
            memory::Static<net::link::FrameBroker> &broker,
            util::Time &time
        )
            : task_executor_{rw, rw, broker, time} {
            auto [f, p] = nb::make_future_promise_pair<bool>();
            initialization_result_ = etl::move(f);
        }

        inline constexpr net::link::AddressTypeSet supported_address_types() const {
            return net::link::AddressTypeSet{net::link::AddressType::Udp};
        }

        inline etl::optional<net::link::Address> broadcast_address() const {
            return etl::nullopt;
        }

        inline net::link::MediaInfo get_media_info() const {
            const auto &address = server_state_.local_address();
            return net::link::MediaInfo{
                .address_type = net::link::AddressType::Udp,
                .address =
                    address.has_value() ? etl::optional(net::link::Address(*address)) : etl::nullopt
            };
        }

        inline void execute(net::frame::FrameService &frame_service, util::Time &time) {
            task_executor_.execute(frame_service, server_state_, time);
        }

        inline nb::Poll<nb::Future<bool>> join_ap(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password,
            util::Time &time
        ) {
            return task_executor_.join_ap(ssid, password, time);
        }

        inline nb::Poll<nb::Future<bool>> start_server(uint16_t port, util::Time &time) {
            return task_executor_.start_udp_server(port, time);
        }

        inline nb::Poll<nb::Future<bool>> close_server(util::Time &time) {
            return task_executor_.close_udp_server(time);
        }
    };
} // namespace media::wifi
