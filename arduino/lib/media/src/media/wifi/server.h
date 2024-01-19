#pragma once

#include "../address/udp.h"
#include <nb/future.h>

namespace media::wifi {
    class LocalServerState {
        etl::optional<UdpIpAddress> local_ip_address_;
        etl::optional<UdpPort> local_port_;
        tl::Optional<nb::Future<UdpIpAddress>> local_ip_address_future_;
        bool is_server_started_{false};

      public:
        inline void set_local_address_future(nb::Future<UdpIpAddress> &&future) {
            local_ip_address_future_ = etl::move(future);
        }

        inline const etl::optional<UdpPort> server_port() const {
            return is_server_started_ ? local_port_ : etl::nullopt;
        }

        inline void on_disconnect_ap() {
            local_ip_address_ = etl::nullopt;
            local_ip_address_future_ = tl::nullopt;
        }

        inline void on_server_started(UdpPort port) {
            is_server_started_ = true;
            local_port_ = port;
        }

        inline void on_server_closed() {
            is_server_started_ = false;
        }

        inline void execute() {
            if (!local_ip_address_future_) {
                return;
            }

            if (auto poll_address = local_ip_address_future_->poll(); poll_address.is_ready()) {
                local_ip_address_ = poll_address.unwrap().get();
            }
        }

        etl::optional<UdpAddress> local_address() const {
            if (!local_ip_address_ || !local_port_) {
                return etl::nullopt;
            }

            return UdpAddress{*local_ip_address_, *local_port_};
        }
    };
} // namespace media::wifi
