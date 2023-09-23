#pragma once

#include "./builder.h"
#include <serde/dec.h>

namespace nb::buf {
    template <typename T>
    struct FormatDecimal final : BufferWriter {
        T value;

        explicit inline FormatDecimal(T value) : value{value} {}

        inline void write_to_builder(BufferBuilder &builder) override {
            builder.append([v = value](auto span) { return serde::dec::serialize(span, v); });
        }
    };

    template <typename T>
    FormatDecimal(T) -> FormatDecimal<T>;
} // namespace nb::buf
