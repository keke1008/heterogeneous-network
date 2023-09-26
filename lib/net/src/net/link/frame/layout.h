#pragma once

#include "./address.h"
#include <nb/future.h>
#include <nb/stream.h>

namespace net::link::frame {
    // フレームのボディ部分の長さを表す型．
    using BodyLength = uint8_t;

    // フレームのボディ部分の長さを表す．
    // UHFモデムの一度に最大送信できるデータ量は255バイトでこれが最小のためこの値となっている．
    inline constexpr BodyLength MTU = 255;

    // フレームのボディ部分の長さを表すために必要なバイト数．
    inline constexpr BodyLength BODY_LENGTH_SIZE = 1;

} // namespace net::link::frame
