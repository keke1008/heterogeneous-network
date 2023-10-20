#pragma once

#include <etl/array.h>
#include <etl/span.h>

namespace net::link::uhf {
    class SerialNumber {
        etl::array<uint8_t, 9> value_;

      public:
        SerialNumber() = delete;
        SerialNumber(const SerialNumber &) = default;
        SerialNumber(SerialNumber &&) = default;
        SerialNumber &operator=(const SerialNumber &) = default;
        SerialNumber &operator=(SerialNumber &&) = default;

        SerialNumber(const etl::span<const uint8_t, 9> span) : value_{} {
            value_.assign(span.begin(), span.end(), 0);
        }

        bool operator==(const SerialNumber &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const SerialNumber &other) const {
            return value_ != other.value_;
        }

        const etl::array<uint8_t, 9> &get() const {
            return value_;
        }
    };
} // namespace net::link::uhf
