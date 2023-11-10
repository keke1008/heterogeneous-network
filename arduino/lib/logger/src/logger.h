#pragma once

#include "./logger/assert.h"
#include "./logger/exception.h"
#include "./logger/halt.h"
#include "./logger/log.h"

namespace logger {
    template <typename T>
    void register_handler(T &serial) {
        exception::register_exception_handler(serial);
        log::register_log_handler(serial);
    }
} // namespace logger
