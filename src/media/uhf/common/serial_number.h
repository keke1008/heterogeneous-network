#pragma once

#include <collection/tiny_buffer.h>

namespace media::uhf {
    class SerialNumber {
        collection::TinyBuffer<uint8_t, 9> value_;

      public:
        SerialNumber() = delete;
        SerialNumber(const SerialNumber &) = default;
        SerialNumber(SerialNumber &&) = default;
        SerialNumber &operator=(const SerialNumber &) = default;
        SerialNumber &operator=(SerialNumber &&) = default;

        SerialNumber(const collection::TinyBuffer<uint8_t, 9> &value) : value_{value} {}

        bool operator==(const SerialNumber &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const SerialNumber &other) const {
            return value_ != other.value_;
        }

        const collection::TinyBuffer<uint8_t, 9> &get() const {
            return value_;
        }
    };
} // namespace media::uhf
