#pragma once

#if __has_include(<Arduino.h>)
#include <Arduino.h>

namespace logger::handler {
    inline HardwareSerial *handler = nullptr;

    inline void register_handler(HardwareSerial &serial) {
        handler = &serial;
    }

    inline HardwareSerial *get_handler() {
        return handler;
    }
} // namespace logger::handler

namespace logger {}

#endif
