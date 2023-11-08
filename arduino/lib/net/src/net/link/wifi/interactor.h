#pragma once

#include "./task.h"
#include "net/link/address.h"

namespace net::link::wifi {
    class WifiInteractor {
        TaskExecutor task_executor_;
        etl::optional<WifiPort> self_port_;
        etl::optional<WifiIpV4Address> self_ip_;

        etl::optional<nb::Future<bool>> initialization_result_;
        etl::optional<nb::Future<WifiIpV4Address>> get_ip_result_;

      public:
        WifiInteractor() = delete;
        WifiInteractor(const WifiInteractor &) = delete;
        WifiInteractor(WifiInteractor &&) = delete;
        WifiInteractor &operator=(const WifiInteractor &) = delete;
        WifiInteractor &operator=(WifiInteractor &&) = delete;

        WifiInteractor(nb::stream::ReadableWritableStream &stream, const FrameBroker &broker)
            : task_executor_{stream, broker} {
            auto [f, p] = nb::make_future_promise_pair<bool>();
            initialization_result_ = etl::move(f);
            task_executor_.emplace<Initialization>(etl::move(p));
        }

        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            return AddressTypeSet{AddressType::IPv4};
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            return AddressTypeSet{};
        }

        inline MediaInfo get_media_info() const {
            etl::optional<WifiAddress> address;
            if (self_ip_.has_value() && self_port_.has_value()) {
                address = WifiAddress{self_ip_.value(), self_port_.value()};
            }

            return MediaInfo{
                .address_type = AddressType::IPv4,
                .address = address ? etl::optional{Address{address.value()}} : etl::nullopt,
            };
        }

        void execute(frame::FrameService &frame_service) {
            auto opt_event = task_executor_.execute(frame_service);
            if (!opt_event) {
                return;
            }

            switch (*opt_event) {
            case WifiEvent::GotIp: {
                auto [f, p] = nb::make_future_promise_pair<WifiIpV4Address>();
                get_ip_result_ = etl::move(f);
                task_executor_.emplace<GetIp>(etl::move(p));
            }
            case WifiEvent::Disconnect: {
                self_ip_.reset();
            }
            }

            if (get_ip_result_.has_value()) {
                auto poll_address = get_ip_result_->poll();
                if (poll_address.is_ready()) {
                    self_ip_ = poll_address.unwrap().get();
                    get_ip_result_.reset();
                }
            }
        }

        inline nb::Poll<nb::Future<bool>>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.emplace<JoinAp>(etl::move(p), ssid, password);
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> start_server(uint16_t port) {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            self_port_ = WifiPort{port};
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.emplace<StartUdpServer>(etl::move(p), WifiPort{port});
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<bool>> close_server() {
            POLL_UNWRAP_OR_RETURN(task_executor_.poll_task_addable());
            auto [f, p] = nb::make_future_promise_pair<bool>();
            task_executor_.emplace<CloseUdpServer>(etl::move(p));
            return etl::move(f);
        }
    };
} // namespace net::link::wifi
