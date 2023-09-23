#pragma once

#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>

namespace util::span {
    inline etl::optional<etl::span<const uint8_t>>
    split_until_byte_exclusive(etl::span<const uint8_t> &bytes, uint8_t byte) {
        for (uint8_t i = 0; i < bytes.size(); ++i) {
            if (bytes[i] == byte) {
                auto result = bytes.subspan(0, i);
                bytes = bytes.subspan(i + 1);
                return result;
            }
        }
        return etl::nullopt;
    }

    inline etl::optional<etl::span<const uint8_t>>
    split_until_byte(etl::span<const uint8_t> &bytes, uint8_t byte) {
        for (uint8_t i = 0; i < bytes.size(); ++i) {
            if (bytes[i] == byte) {
                auto result = bytes.subspan(0, i + 1);
                bytes = bytes.subspan(i + 1);
                return result;
            }
        }
        return etl::nullopt;
    }
} // namespace util::span
