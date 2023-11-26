#pragma once

#if __has_include(<Arduino.h>)

#include <undefArduino.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wvolatile"

// 0.5秒に1回LEDを点滅させる
namespace board::blink {
    inline void setup() {
        TCCR1A = 0;
        TCCR1B = 0;
        TCNT1 = 0;

        OCR1A = 7812;
        TCCR1B |= (1 << WGM12);
        TIMSK1 |= (1 << OCIE1A);

        pinMode(LED_BUILTIN, OUTPUT);
    }

    inline void blink() {
        TCCR1B |= (1 << CS12) | (1 << CS10);
    }

    inline void stop() {
        TCCR1B &= ~((1 << CS12) | (1 << CS10));
    }

    ISR(TIMER1_COMPA_vect) {
        static bool ledState = false;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    }
} // namespace board::blink

#pragma GCC diagnostic pop

#else

namespace board::blink {
    inline void setup() {}

    inline void blink() {}

    inline void stop() {}
} // namespace board::blink
#endif
