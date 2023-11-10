#pragma once

#if __has_include(<Arduino.h>)
#include <Arduino.h>

#define FLASH_STRING(string_literal)                                                               \
    (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

namespace util {
    using FlashStringType = const __FlashStringHelper *;
}

#else

#define FLASH_STRING(string_literal) string_literal

namespace util {
    using FlashStringType = const char *;
}

#endif
