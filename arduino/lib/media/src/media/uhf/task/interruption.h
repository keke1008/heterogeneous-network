#pragma once

#include <stdint.h>

namespace media::uhf {
    enum class TaskInterruptionResult : uint8_t {
        Interrupted,
        Aborted,
    };
} // namespace media::uhf
