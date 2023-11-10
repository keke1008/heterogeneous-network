#pragma once

#include "./log/assert.h"
#include "./log/exception.h"
#include "./log/halt.h"
#include "./log/log.h"

namespace logging {
    template <typename T>
    void register_handler(T &serial) {
        exception::register_exception_handler(serial);
        log::register_log_handler(serial);
    }
} // namespace logging
