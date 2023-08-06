#include "./time.h"

#if __has_include(<Arduino.h>)

#include <Arduino.h>

namespace util {
    Instant Time::now() const {
        return Instant{millis()};
    }
} // namespace util

#else

#include <chrono>

namespace util {
    Instant Time::now() const {
        auto now = std::chrono::steady_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return Instant{static_cast<TimeInt>(millis.count())};
    }
} // namespace util

#endif

namespace util {
    Instant MockTime::now() const {
        return now_;
    }

    void MockTime::set_now(TimeInt now_ms) {
        now_ = Instant{now_ms};
    }

    TimeInt MockTime::get_now() const {
        return now_.ms_;
    }
} // namespace util
