#pragma once

#include "./board/blink.h"
#include "./board/random_seed.h"

namespace board {
    inline void setup() {
        blink::setup();
        random_seed::setup();
    }
} // namespace board
