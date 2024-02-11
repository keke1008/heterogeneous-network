#pragma once

#include <etl/type_traits.h>
#include <stdint.h>

#if __has_include(<Arduino.h>)
#include <Arduino.h>

namespace util {
    using FlashStringType = const __FlashStringHelper *;

    inline uint8_t read_flash_byte(FlashStringType flash_string) {
        return pgm_read_byte(flash_string);
    }

    inline uint8_t read_flash_byte_offset(FlashStringType flash_string, size_t offset) {
        const char *head = reinterpret_cast<const char *>(flash_string);
        return pgm_read_byte(head + offset);
    }

#define FLASH_STRING(string_literal) (reinterpret_cast<util::FlashStringType>(PSTR(string_literal)))

} // namespace util

#else

namespace util {
    class __FlashStringHelper;
    using FlashStringType = const __FlashStringHelper *;

    inline uint8_t read_flash_byte(FlashStringType flash_string) {
        return *reinterpret_cast<const uint8_t *>(flash_string);
    }

    inline uint8_t read_flash_byte_offset(FlashStringType flash_string, size_t offset) {
        return *(reinterpret_cast<const uint8_t *>(flash_string) + offset);
    }
} // namespace util

#define FLASH_STRING(string_literal) (reinterpret_cast<util::FlashStringType>(string_literal))

#endif
