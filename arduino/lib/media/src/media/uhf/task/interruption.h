#pragma once

#include <stdint.h>

namespace net::link::uhf {
    enum class TaskInterruptionResult : uint8_t {
        Interrupted,
        Aborted,
    };
} // namespace net::link::uhf
