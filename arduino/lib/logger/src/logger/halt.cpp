#include "./halt.h"

#include "./handler.h"
#include <util/flash_string.h>

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logger {
    [[noreturn]] void halt() {
        auto handler = handler::get_handler();
        if (handler != nullptr) {
            handler->println(FLASH_STRING("Program halted"));
        }

        pinMode(LED_BUILTIN, OUTPUT);

        constexpr uint8_t DELAY = 250;
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(DELAY);
            digitalWrite(LED_BUILTIN, LOW);
            delay(DELAY);
        }
    }
} // namespace logger

#else

namespace logger {
    [[noreturn]] void halt() {
        throw "Program halted";
    }
} // namespace logger

#endif
