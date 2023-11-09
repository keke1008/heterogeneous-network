#pragma once

#include <etl/limits.h>
#include <etl/span.h>
#include <log.h>

namespace serde::dec {
    namespace {
        template <typename T>
        inline constexpr T max_digit = etl::numeric_limits<T>::digits10 + 1;

        template <typename T>
        inline constexpr T pow10(T n) {
            return n == 0 ? 1 : 10 * pow10(n - 1);
        }

    } // namespace

    template <typename T>
    inline uint8_t serialize(etl::span<uint8_t> span, T value) {
        ASSERT(span.size() >= max_digit<T>);

        if (value == 0) {
            span[0] = '0';
            return 1;
        }

        bool zero = true;
        uint8_t index = 0;
        constexpr T max_rem = pow10(max_digit<T> - 1);
        for (T rem = max_rem; rem != 0; rem /= 10) {
            T div = value / rem;
            value -= div * rem;

            if (div != 0 || !zero) {
                span[index++] = div + '0';
                zero = false;
            }
        }

        return index;
    }

    template <typename T>
    inline T deserialize(const etl::span<const uint8_t> span) {
        ASSERT(span.size() <= max_digit<T>);

        T result = 0;
        for (uint8_t value : span) {
            ASSERT(value >= '0' && value <= '9');
            result *= 10;
            result += value - '0';
        }
        return result;
    }
} // namespace serde::dec
