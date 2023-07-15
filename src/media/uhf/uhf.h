#pragma once

#include "./modem.h"
#include <etl/optional.h>
#include <media/common.h>
#include <nb/serial.h>

namespace media {
    template <typename T>
    class UHF {
        nb::serial::Serial<T> serial_;
    };
} // namespace media
