#pragma once

#include <debug_assert.h>
#include <etl/span.h>
#include <util/concepts.h>

namespace serde::bin {
    template <util::unsigned_integral T>
    inline constexpr uint8_t serialize(etl::span<uint8_t> buffer, T value) {
        DEBUG_ASSERT(buffer.size() >= sizeof(T));
        for (uint8_t i = 0; i < sizeof(T); ++i) {
            buffer[i] = value >> (i * 8) & 0xFF;
        }
        return sizeof(T);
    }

    template <util::unsigned_integral T>
    inline constexpr T deserialize(etl::span<const uint8_t> buffer) {
        DEBUG_ASSERT(buffer.size() >= sizeof(T));
        T value = 0;
        for (uint8_t i = 0; i < sizeof(T); ++i) {
            value |= static_cast<T>(buffer[i]) << (i * 8);
        }
        return value;
    }
} // namespace serde::bin
