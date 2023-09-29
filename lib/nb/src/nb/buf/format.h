#pragma once

#include "./builder.h"
#include <serde/bin.h>
#include <serde/dec.h>
#include <util/concepts.h>

namespace nb::buf {
    template <util::unsigned_integral T>
    struct FormatDecimal final : BufferWriter {
        T value;

        explicit inline FormatDecimal(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) override {
            builder.append([v = value](auto span) { return serde::dec::serialize(span, v); });
        }
    };

    template <util::unsigned_integral T>
    FormatDecimal(T) -> FormatDecimal<T>;

    template <util::unsigned_integral T>
    struct FormatBinary final : BufferWriter {
        T value;

        explicit inline FormatBinary(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) override {
            builder.append([v = value](auto span) { return serde::bin::serialize(span, v); });
        }
    };

    template <util::unsigned_integral T>
    FormatBinary(T) -> FormatBinary<T>;
} // namespace nb::buf
