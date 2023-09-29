#pragma once

#include <debug_assert.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/string_view.h>

namespace util {
    namespace [[deprecated]] span {
        [[deprecated("Use util::as_str() and == instead")]] inline constexpr bool
        equal(etl::span<const uint8_t> bytes, etl::string_view str) {
            if (bytes.size() != str.size()) {
                return false;
            }
            const uint8_t *str_begin = reinterpret_cast<const uint8_t *>(str.data());
            return etl::equal(bytes.begin(), bytes.end(), str_begin);
        }

        [[deprecated("Use nb::buf::BufferSplitter::split_sentinel() instead"
        )]] inline etl::optional<etl::span<const uint8_t>>
        take_until(etl::span<const uint8_t> &bytes, uint8_t byte) {
            for (uint8_t i = 0; i < bytes.size(); ++i) {
                if (bytes[i] == byte) {
                    auto result = bytes.subspan(0, i);
                    bytes = bytes.subspan(i + 1);
                    return result;
                }
            }
            return etl::nullopt;
        }
    } // namespace span
} // namespace util

namespace util {
    inline etl::span<const uint8_t> as_bytes(const etl::string_view str) {
        return etl::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()), str.size());
    }

    inline const etl::string_view as_str(const etl::span<const uint8_t> bytes) {
        return etl::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    }
} // namespace util
