#pragma once

#include "./builder.h"
#include <serde/bin.h>
#include <serde/dec.h>
#include <serde/hex.h>
#include <util/concepts.h>

namespace nb::buf {
    template <util::unsigned_integral T>
    struct FormatDecimal {
        T value;

        explicit inline FormatDecimal(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) {
            builder.append([v = value](auto span) { return serde::dec::serialize(span, v); });
        }
    };

    template <util::unsigned_integral T>
    FormatDecimal(T) -> FormatDecimal<T>;

    template <util::unsigned_integral T>
    struct FormatBinary {
        T value;

        explicit inline FormatBinary(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) {
            builder.append([v = value](auto span) { return serde::bin::serialize(span, v); });
        }
    };

    template <util::unsigned_integral T>
    FormatBinary(T) -> FormatBinary<T>;

    template <util::unsigned_integral T>
    struct FormatHexaDecimal {
        T value;

        explicit inline FormatHexaDecimal(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) {
            builder.append([v = value](auto span) {
                auto result = serde::hex::serialize<T>(v);
                etl::copy_n(result.begin(), result.size(), span.begin());
                return static_cast<uint8_t>(result.size());
            });
        }
    };

    template <util::unsigned_integral T>
    FormatHexaDecimal(T) -> FormatHexaDecimal<T>;
} // namespace nb::buf
