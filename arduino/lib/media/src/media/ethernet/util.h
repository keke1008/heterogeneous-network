#pragma once

#include <Ethernet.h>

#include "../address/udp.h"
#include <etl/span.h>

namespace media::ethernet {
    UdpAddress ip_and_port_to_udp_address(IPAddress &ip, uint16_t port) {
        UdpIpAddress udp_ip{etl::span{&ip[0], 4}};
        UdpPort udp_port{port};
        return UdpAddress{udp_ip, udp_port};
    }

    inline UdpAddress ip_and_port_to_udp_address(IPAddress &&ip, uint16_t port) {
        return ip_and_port_to_udp_address(ip, port);
    }

    inline IPAddress udp_address_to_ip(const UdpAddress &address) {
        return IPAddress{address.address().body().begin()};
    }

    inline uint16_t udp_address_to_port(const UdpAddress &address) {
        return address.port().value();
    }
} // namespace media::ethernet
