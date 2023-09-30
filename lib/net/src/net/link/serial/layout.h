#pragma once

#include "../address.h"
#include "./address.h"
#include <net/frame/fields.h>
#include <stdint.h>

namespace net::link::serial {
    // # シリアル通信のフレームレイアウト
    //
    // 1. プリアンブル(8byte)
    // 2. 上位プロトコル(1byte)
    // 3. 送信元アドレス(1byte)
    // 4. 宛先アドレス(1byte)
    // 5. データ長(1byte)
    // 6. データ

    constexpr uint8_t HEADER_LENGTH =
        frame::PROTOCOL_SIZE + SerialAddress::SIZE * 2 + frame::BODY_LENGTH_SIZE;

    constexpr uint8_t PREAMBLE = 0b10101010;
    constexpr uint8_t PREAMBLE_LENGTH = 8;
} // namespace net::link::serial
