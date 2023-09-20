#include "./time.h"

#if __has_include(<Arduino.h>)

#include <Arduino.h>

namespace util {
    Instant Time::now() const {
        return Instant{millis()};
    }
} // namespace util

#endif
