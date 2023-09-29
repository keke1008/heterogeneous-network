#pragma once

#include "../frame.h"
#include "./address.h"
#include <stdint.h>

namespace net::link::serial {
    // # シリアル通信のフレームレイアウト
    //
    // 1. プリアンブル(8byte)
    // 2. 宛先アドレス(1byte)
    // 3. 送信元アドレス(1byte)
    // 4. フレーム長(1byte)
    // 5. フレーム(<= 255byte)

    constexpr uint8_t HEADER_LENGTH = SerialAddress::SIZE * 2 + frame::BODY_LENGTH_SIZE;

    constexpr uint8_t PREAMBLE = 0b10101010;
    constexpr uint8_t PREAMBLE_LENGTH = 8;
} // namespace net::link::serial
