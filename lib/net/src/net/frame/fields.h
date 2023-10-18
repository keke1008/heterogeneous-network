#pragma once

#include <stdint.h>

// link層のプロトコルは以下のパラメータを送受信できる必要がある．
// 1. フレームのボディ部分の長さ
// 2. フレームの上位プロトコル
// 3. 送信元のアドレス
// 4. 宛先のアドレス
// 5. フレームのボディ部分
namespace net::frame {
    // フレームのボディ部分の長さを表す．
    // link層のプロトコルは最大で255バイトのデータを送信できる必要がある．
    // UHFモデムの一度に最大送信できるデータ量は255バイトだが，
    // 上位プロトコルを指定するため1byte少なくなっている．
    inline constexpr uint8_t MTU = 254;

    // フレームのボディ部分の長さを表すために必要なバイト数．
    inline constexpr uint8_t BODY_LENGTH_SIZE = 1;

    // フレームの上位プロトコルを指定するために必要なバイト数．
    inline constexpr uint8_t PROTOCOL_SIZE = 1;
} // namespace net::frame
