#pragma once

#include <stdint.h>

namespace net::protocol::protocol_number {
    constexpr uint8_t TRANSMISSION_PRE = 0x01;
    constexpr uint8_t TRANSMISSION_DATA = 0x02;
    constexpr uint8_t TRANSMISSION_ACK = 0x03;
    constexpr uint8_t TRANSMISSION_NACK = 0x04;
} // namespace net::protocol::protocol_number
