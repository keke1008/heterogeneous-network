#pragma once

#include <doctest.h>
#include <util/span.h> // re-export

#include <etl/span.h>
#include <etl/string_view.h>
#include <iostream>
#include <string_view>

namespace etl {
    inline std::ostream &operator<<(std::ostream &os, const etl::string_view &str) {
        os << std::string_view(reinterpret_cast<const char *>(str.data()), str.size());
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const etl::span<const uint8_t> &bytes) {
        os << std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        return os;
    }
} // namespace etl
