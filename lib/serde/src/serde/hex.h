#pragma once

#include <etl/array.h>
#include <etl/optional.h>
#include <etl/span.h>

namespace serde::hex {
    template <typename T>
    inline constexpr bool is_unsigned_integral_v =
        etl::conjunction_v<etl::is_integral<T>, etl::is_unsigned<T>>;

    constexpr uint8_t to_hex_char(uint8_t value) {
        return value < 10 ? value + '0' : value - 10 + 'A';
    }

    template <typename T>
    etl::enable_if_t<is_unsigned_integral_v<T>, etl::array<uint8_t, sizeof(T) * 2>>
    serialize(T value) {
        constexpr uint8_t length = static_cast<uint8_t>(sizeof(T)) * 2;
        etl::array<uint8_t, length> result{};
        for (uint8_t i = 0; i < length; ++i) {
            result[length - i - 1] = to_hex_char((value >> (i * 4)) & 0x0F);
        }
        return result;
    }

    constexpr etl::optional<uint8_t> from_hex_char(uint8_t ch) {
        if ('0' <= ch && ch <= '9') {
            return ch - '0';
        } else if ('A' <= ch && ch <= 'F') {
            return ch - 'A' + 10;
        }
        return etl::nullopt;
    }

    template <typename T>
    etl::enable_if_t<is_unsigned_integral_v<T>, etl::optional<T>>
    deserialize(const etl::span<uint8_t, sizeof(T) * 2> &data) {
        T result = 0;
        uint8_t length = static_cast<uint8_t>(sizeof(T)) * 2;
        for (uint8_t i = 0; i < length; ++i) {
            auto value = from_hex_char(data[length - i - 1]);
            if (!value) {
                return etl::nullopt;
            }
            result |= static_cast<T>(*value) << (i * 4);
        }
        return result;
    }
} // namespace serde::hex
