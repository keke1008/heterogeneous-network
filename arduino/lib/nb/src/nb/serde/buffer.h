#pragma once

#include "./de.h"
#include "./ser.h"

namespace nb {
    template <typename T>
    concept AsyncWritableBuffer = ser::AsyncWritable<T> && requires(T &t, uint8_t length) {
        { t.writable_length() } -> tl::same_as<uint8_t>;
        { t.write_buffer_unchecked(length) } -> tl::same_as<etl::span<uint8_t>>;
    };

    template <typename T>
    concept AsyncReadableBuffer = de::AsyncReadable<T> && requires(T &t, uint8_t length) {
        { t.readable_length() } -> tl::same_as<uint8_t>;
        { t.read_buffer_unchecked(length) } -> tl::same_as<etl::span<const uint8_t>>;
    };

} // namespace nb
