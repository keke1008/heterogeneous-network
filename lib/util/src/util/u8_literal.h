#pragma once

#include <etl/array.h>
#include <stddef.h>
#include <stdint.h>

namespace util {
    namespace [[deprecated]] u8_literal {
        class [[deprecated]] U8Iterable {
            const char *begin_;
            const char *end_;

          public:
            inline constexpr U8Iterable(const char *begin, size_t len)
                : begin_{begin},
                  end_{begin + len} {}

            inline const uint8_t *begin() const {
                return reinterpret_cast<const uint8_t *>(begin_);
            }

            inline const uint8_t *end() const {
                return reinterpret_cast<const uint8_t *>(end_);
            }

            constexpr size_t size() const {
                return end_ - begin_;
            }
        };

        [[deprecated]] inline constexpr U8Iterable operator""_u8it(const char *str, size_t len) {
            return U8Iterable{str, len};
        }

        template <typename T, T... Str>
        [[deprecated]] inline constexpr etl::array<uint8_t, sizeof...(Str)> operator""_u8array() {
            return {static_cast<uint8_t>(Str)...};
        }

        [[deprecated]] inline constexpr uint8_t operator""_u8(char ch) {
            return static_cast<uint8_t>(ch);
        }
    } // namespace u8_literal
} // namespace util
