#pragma once

#if __has_include(<Arduino.h>)

#include <undefArduino.h>

namespace board::random_seed {
    inline void setup() {
        randomSeed(analogRead(0));
    }
} // namespace board::random_seed

#else

namespace board::random_seed {
    inline void setup() {}
} // namespace board::random_seed
#endif
