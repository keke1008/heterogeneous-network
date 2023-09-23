#pragma once

#include <debug_assert.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/string_view.h>

namespace util::span {
    inline constexpr bool equal(etl::span<const uint8_t> bytes, etl::string_view str) {
        if (bytes.size() != str.size()) {
            return false;
        }
        const uint8_t *str_begin = reinterpret_cast<const uint8_t *>(str.data());
        return etl::equal(bytes.begin(), bytes.end(), str_begin);
    }

    inline constexpr bool starts_with(etl::span<const uint8_t> bytes, etl::string_view prefix) {
        if (bytes.size() < prefix.size()) {
            return false;
        }
        return equal(bytes.subspan(0, prefix.size()), prefix);
    }

    inline constexpr bool ends_with(etl::span<const uint8_t> bytes, etl::string_view suffix) {
        if (bytes.size() < suffix.size()) {
            return false;
        }
        return equal(bytes.last(suffix.size()), suffix);
    }

    inline constexpr etl::span<const uint8_t> drop_crlf(etl::span<const uint8_t> bytes) {
        if (ends_with(bytes, "\r\n")) {
            return bytes.first(bytes.size() - 2);
        }
        return bytes;
    }

    inline etl::optional<etl::span<const uint8_t>>
    split_until_byte_exclusive(etl::span<const uint8_t> &bytes, uint8_t byte) {
        for (uint8_t i = 0; i < bytes.size(); ++i) {
            if (bytes[i] == byte) {
                auto result = bytes.subspan(0, i);
                bytes = bytes.subspan(i + 1);
                return result;
            }
        }
        return etl::nullopt;
    }

    inline etl::optional<etl::span<const uint8_t>>
    split_until_byte(etl::span<const uint8_t> &bytes, uint8_t byte) {
        for (uint8_t i = 0; i < bytes.size(); ++i) {
            if (bytes[i] == byte) {
                auto result = bytes.subspan(0, i + 1);
                bytes = bytes.subspan(i + 1);
                return result;
            }
        }
        return etl::nullopt;
    }

    template <uint8_t N>
    inline etl::optional<etl::array<etl::span<const uint8_t>, N>>
    splitn_by(etl::span<const uint8_t> bytes, uint8_t byte) {
        etl::array<etl::span<const uint8_t>, N> result;
        for (uint8_t i = 0; i < N; ++i) {
            auto split = split_until_byte(bytes, byte);
            if (!split.has_value()) {
                return etl::nullopt;
            }
            result[i] = *split;
        }
        return result;
    }
} // namespace util::span
