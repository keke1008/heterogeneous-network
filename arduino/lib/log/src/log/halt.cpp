#include "./halt.h"

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logging {
    [[noreturn]] void halt() {
        pinMode(LED_BUILTIN, OUTPUT);

        constexpr uint8_t DELAY = 250;
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(DELAY);
            digitalWrite(LED_BUILTIN, LOW);
            delay(DELAY);
        }
    }
} // namespace logging

#else

namespace logging {
    [[noreturn]] void halt() {
        throw "Program halted";
    }
} // namespace logging

#endif
