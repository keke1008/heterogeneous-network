#pragma once

#if __has_include(<Arduino.h>)

#include <Arduino.h>

namespace util {
    class Rand {
      public:
        template <typename T>
        T gen(T max) {
            static_cast<T>(random(max));
        }

        template <typename T>
        T gen(T min, T max) {
            static_cast<T>(random(min, max));
        }
    };
} // namespace util

#else

#include <random>

namespace util {
    class Rand {
        std::mt19937 gen_{};

      public:
        template <typename T>
        T gen(T max) {
            std::uniform_int_distribution<T> dis(0, max);
            return dis(gen_);
        }

        template <typename T>
        T gen(T min, T max) {
            std::uniform_int_distribution<T> dis(min, max);
            return dis(gen_);
        }
    };
} // namespace util

#endif

namespace util {
    class MockRandom {
        long value_;

      public:
        MockRandom(long value) : value_{value} {}

        template <typename T>
        T gen(T max) {
            return value_;
        }

        template <typename T>
        T gen(T min, T max) {
            return value_;
        }

        void set_value(long value) {
            value_ = value;
        }

        long get_value() const {
            return value_;
        }
    };
} // namespace util
