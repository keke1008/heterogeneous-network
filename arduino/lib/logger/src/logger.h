#pragma once

// IWYU pragma: begin_exports
#include "./logger/assert.h"
#include "./logger/exception.h"
#include "./logger/halt.h"
#include "./logger/log.h"
// IWYU pragma: end_exports

#if __has_include(<Arduino.h>)

namespace logger {
    template <typename T>
    void register_handler(T &serial) {
        exception::register_exception_handler();
        handler::register_handler(serial);
    }
} // namespace logger

#else

namespace logger {
    template <typename T>
    void register_handler(T &) {}
} // namespace logger

#endif
