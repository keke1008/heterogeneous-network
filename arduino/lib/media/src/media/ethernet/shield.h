#pragma once

#include <Ethernet.h>
#include <EthernetUdp.h>

#include "../address/udp.h"
#include "./constants.h"
#include "./util.h"
#include <net.h>
#include <util/rand.h>

namespace media::ethernet {
    etl::array<uint8_t, 6> generate_randomized_mac_address(util::Rand &rand) {
        etl::array<uint8_t, 6> mac;

        // ランダムなバイト列を生成する
        for (auto &byte : mac) {
            byte = rand.gen_uint8_t();
        }

        // ローカルアドレスのためのビットを立てる
        mac[0] |= 0b00000010;

        // ユニキャストアドレスのためのビットを落とす
        mac[0] &= 0b11111110;

        return mac;
    }

    inline EthernetUDP udp;
    inline bool udp_initialized = false;

    enum class LinkState : uint8_t {
        JustDown,
        Up,
        Down,
    };

    class EthernetShield {
        bool has_ethernet_shield_ = false;
        bool has_valid_address = false;
        bool is_link_up_ = false;
        nb::Debounce check_link_up_debounce_;

        inline void update_link_up() {
            auto link_status = Ethernet.linkStatus();
            is_link_up_ = link_status == LinkON;
        }

      public:
        explicit EthernetShield(util::Time &time)
            : check_link_up_debounce_{time, CHECK_LINK_UP_INTERVAL} {}

        void initialize(util::Rand &rand) {
            // 複数回の初期化を防ぐ
            FASSERT(!udp_initialized);
            udp_initialized = true;

            // Ethernet の初期化
            auto mac = generate_randomized_mac_address(rand);
            uint8_t dhcp_success = Ethernet.begin(mac.begin(), SETUP_TIMEOUT.millis());
            if (dhcp_success == 0) {
                LOG_INFO(FLASH_STRING("Failed to configure Ethernet using DHCP"));
                has_valid_address = false;
            } else {
                has_valid_address = true;
            }

            // Ethernet シールド が接続されているか確認する
            auto hardware_status = Ethernet.hardwareStatus();
            if (hardware_status == EthernetNoHardware) {
                LOG_INFO(FLASH_STRING("Ethernet shield was not found"));
                has_ethernet_shield_ = false;
                return;
            } else {
                has_ethernet_shield_ = true;
            }

            LOG_INFO(FLASH_STRING("Ethernet shield was found"));

            // Ethernet シールド がリンクアップしているか確認する
            auto link_status = Ethernet.linkStatus();
            is_link_up_ = link_status == LinkON;

            // UDP の初期化
            udp.begin(UDP_PORT);
        }

        inline etl::optional<UdpAddress> get_local_address() const {
            FASSERT(udp_initialized);

            if (has_valid_address) {
                return ip_and_port_to_udp_address(Ethernet.localIP(), udp.localPort());
            } else {
                return etl::nullopt;
            }
        }

        inline net::link::MediaPortOperationResult set_local_ip_address(etl::span<const uint8_t> ip
        ) {
            FASSERT(udp_initialized);

            if (!has_ethernet_shield_) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            IPAddress address{ip.data()};
            Ethernet.setLocalIP(address);

            has_valid_address = true;
            return net::link::MediaPortOperationResult::Success;
        }

        inline net::link::MediaPortOperationResult set_subnet_mask(etl::span<const uint8_t> mask) {
            FASSERT(udp_initialized);

            if (!has_ethernet_shield_) {
                return net::link::MediaPortOperationResult::UnsupportedOperation;
            }

            IPAddress address{mask.data()};
            Ethernet.setSubnetMask(address);
            return net::link::MediaPortOperationResult::Success;
        }

        inline LinkState execute(util::Time &time) {
            FASSERT(udp_initialized);

            if (!has_ethernet_shield_) {
                return LinkState::Down;
            }

            if (check_link_up_debounce_.poll(time).is_pending()) {
                return is_link_up_ ? LinkState::Up : LinkState::Down;
            }

            bool prev_link_up = is_link_up_;
            auto link_state = Ethernet.linkStatus();
            is_link_up_ = link_state == LinkON;

            if (prev_link_up && !is_link_up_) {
                return LinkState::JustDown;
            } else {
                return is_link_up_ ? LinkState::Up : LinkState::Down;
            }
        }
    };
} // namespace media::ethernet
