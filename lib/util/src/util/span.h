#pragma once

#include <debug_assert.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/string_view.h>

namespace util {
    inline etl::span<const uint8_t> as_bytes(const etl::string_view str) {
        return etl::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()), str.size());
    }

    inline const etl::string_view as_str(const etl::span<const uint8_t> bytes) {
        return etl::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    }
} // namespace util
