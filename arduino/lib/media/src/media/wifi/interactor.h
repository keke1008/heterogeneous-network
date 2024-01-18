#pragma once

#include "./task.h"
#include "net/link/address.h"

namespace net::link::wifi {
    template <nb::AsyncReadableWritable RW>
    class WifiInteractor {
        TaskExecutor<RW> task_executor_;
        LocalServerState server_state_;

        etl::optional<nb::Future<bool>> initialization_result_;
        etl::optional<nb::Future<WifiIpV4Address>> get_ip_result_;

      public:
        WifiInteractor() = delete;
        WifiInteractor(const WifiInteractor &) = delete;
        WifiInteractor(WifiInteractor &&) = delete;
        WifiInteractor &operator=(const WifiInteractor &) = delete;
        WifiInteractor &operator=(WifiInteractor &&) = delete;

        WifiInteractor(RW &stream, const FrameBroker &broker, util::Time &time)
            : task_executor_{stream, broker} {
            auto [f, p] = nb::make_future_promise_pair<bool>();
            initialization_result_ = etl::move(f);
            task_executor_.template emplace_task<Initialization>(time, etl::move(p));
        }

        inline constexpr AddressTypeSet supported_address_types() const {
            return AddressTypeSet{AddressType::IPv4};
        }

        inline etl::optional<Address> broadcast_address() const {
            return etl::nullopt;
        }

        inline MediaInfo get_media_info() const {
            const auto &address = server_state_.global_address();
            return MediaInfo{
                .address_type = AddressType::IPv4,
                .address = address ? etl::optional{Address{address.value()}} : etl::nullopt,
            };
        }

        inline void execute(frame::FrameService &frame_service, util::Time &time) {
            task_executor_.execute(frame_service, server_state_, time);
        }

        inline nb::Poll<nb::Future<bool>> join_ap(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password,
            util::Time &time
        ) {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.template emplace_task<JoinAp>(time, etl::move(p), ssid, password);
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> start_server(uint16_t port, util::Time &time) {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            server_state_.on_server_started(WifiPort{port});
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.template emplace_task<StartUdpServer>(
                time, etl::move(p), WifiPort{port}
            );
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> close_server(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.template emplace_task<CloseUdpServer>(time, etl::move(p));
            return etl::move(f);
        }
    };
} // namespace net::link::wifi
