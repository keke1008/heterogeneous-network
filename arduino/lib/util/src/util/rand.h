#pragma once

#include <etl/limits.h>
#include <stdint.h>

namespace util {
    class Rand {
      public:
        virtual uint8_t gen_uint8_t(uint8_t max = etl::integral_limits<uint8_t>::max) = 0;
        virtual uint8_t gen_uint8_t(uint8_t min, uint8_t max) = 0;

        virtual uint16_t gen_uint16_t(uint16_t max = etl::integral_limits<uint16_t>::max) = 0;
        virtual uint16_t gen_uint16_t(uint16_t min, uint16_t max) = 0;

        virtual uint32_t gen_uint32_t(uint32_t max = etl::integral_limits<uint32_t>::max) = 0;
        virtual uint32_t gen_uint32_t(uint32_t min, uint32_t max) = 0;
    };
} // namespace util

#if __has_include(<Arduino.h>)

#include <undefArduino.h>

namespace util {
#define DEFINE_GEN(TYPE)                                                                           \
    TYPE gen_##TYPE(TYPE max = etl::integral_limits<TYPE>::max) override {                         \
        return random(max);                                                                        \
    }                                                                                              \
    TYPE gen_##TYPE(TYPE min, TYPE max) override {                                                 \
        return random(min, max);                                                                   \
    }

    class ArduinoRand final : public Rand {
      public:
        DEFINE_GEN(uint8_t);
        DEFINE_GEN(uint16_t);
        DEFINE_GEN(uint32_t);
    };

#undef DEFINE_GEN
} // namespace util

#endif

namespace util {
#define DEFINE_GEN(TYPE)                                                                           \
    TYPE gen_##TYPE(TYPE max = etl::integral_limits<TYPE>::max) override {                         \
        return value_;                                                                             \
    }                                                                                              \
    TYPE gen_##TYPE(TYPE min, TYPE max) override {                                                 \
        return value_;                                                                             \
    }

    class MockRandom final : public Rand {
        uint32_t value_;

      public:
        MockRandom(uint32_t value) : value_{value} {}

        DEFINE_GEN(uint8_t);
        DEFINE_GEN(uint16_t);
        DEFINE_GEN(uint32_t);
    };

#undef DEFINE_GEN
} // namespace util
