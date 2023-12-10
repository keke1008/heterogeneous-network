#pragma once

#include "./frame.h"
#include <nb/future.h>

namespace net::link::wifi {
    class LocalServerState {
        etl::optional<WifiIpV4Address> local_ip_address_;
        etl::optional<WifiPort> local_port_;
        etl::optional<WifiAddress> global_address_;
        tl::Optional<nb::Future<WifiIpV4Address>> local_ip_address_future_;
        bool is_server_started_{false};

        inline void remove_invalid_global_address() {
            if (!local_ip_address_ || !local_port_ || !global_address_) {
                return;
            }

            if (global_address_->address_part() == *local_ip_address_ &&
                global_address_->port_part() == *local_port_) {
                global_address_ = etl::nullopt;
            }
        }

      public:
        inline void set_local_address_future(nb::Future<WifiIpV4Address> &&future) {
            local_ip_address_future_ = etl::move(future);
        }

        inline bool has_global_address() const {
            return global_address_.has_value();
        }

        inline const etl::optional<WifiAddress> &global_address() const {
            return global_address_;
        }

        inline const etl::optional<WifiPort> server_port() const {
            return is_server_started_ ? local_port_ : etl::nullopt;
        }

        inline void on_got_maybe_global_ip(const WifiAddress &address) {
            global_address_ = address;
            remove_invalid_global_address();
        }

        inline void on_disconnect_ap() {
            local_ip_address_ = etl::nullopt;
            global_address_ = etl::nullopt;
            local_ip_address_future_ = tl::nullopt;
        }

        inline void on_server_started(WifiPort port) {
            is_server_started_ = true;
            local_port_ = port;
            remove_invalid_global_address();
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
    };
} // namespace net::link::wifi
