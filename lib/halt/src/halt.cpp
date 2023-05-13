#include "halt.hpp"

#ifdef NATIVE_ENVIRONMENT

#include <exception>
#include <stdexcept>

[[noreturn]] void halt() {
    throw std::runtime_error("An error occurred. Program halted.");
}

#else

#include <Arduino.h>

const uint8_t ON_BOARD_LED_PIN = 13;

[[noreturn]] void halt() {
    pinMode(ON_BOARD_LED_PIN, OUTPUT);

    while (1) {
        digitalWrite(ON_BOARD_LED_PIN, HIGH);
        delay(500);
        digitalWrite(ON_BOARD_LED_PIN, LOW);
        delay(500);
    }
}

#endif
