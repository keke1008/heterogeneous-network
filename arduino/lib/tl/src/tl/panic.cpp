#include "./panic.h"

#if __has_include(<Arduino.h>)

namespace tl {
    [[noreturn]] void panic() {
        while (true) {}
    }
} // namespace tl

#else

namespace tl {
    [[noreturn]] void panic() {
        throw "panic";
    }
} // namespace tl

#endif
