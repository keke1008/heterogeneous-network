#pragma once

#include <etl/array.h>

namespace bytes {

    template <typename T>
    inline constexpr bool is_unsigned_integral_v =
        etl::conjunction_v<etl::is_integral<T>, etl::is_unsigned<T>>;

    template <typename T>
    constexpr inline etl::array<uint8_t, sizeof(T)> serialize(T value) {
        static_assert(is_unsigned_integral_v<T>);

        etl::array<uint8_t, sizeof(T)> result;
        uint8_t size = static_cast<uint8_t>(sizeof(T));

        for (uint8_t i = 0; i < size; i++) {
            result[i] = value >> (i * 8) & 0xFF;
        }
        return result;
    }

    template <typename T>
    constexpr inline T deserialize(const etl::array<uint8_t, sizeof(T)> &data) {
        static_assert(is_unsigned_integral_v<T>);

        T result{0};
        uint8_t size = static_cast<uint8_t>(sizeof(T));
        for (uint8_t i = 0; i < size; i++) {
            result |= data[i] << (i * 8);
        }
        return result;
    }
} // namespace bytes
