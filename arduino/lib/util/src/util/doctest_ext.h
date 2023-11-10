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

REGISTER_EXCEPTION_TRANSLATOR(etl::exception &e) {
    return doctest::String("'") + e.what() + "' in " + e.file_name() + ":" +
        doctest::toString(e.line_number());
};

struct DestroyCount {
    int *ptr;
    int value_;

    explicit DestroyCount(int &ptr) : ptr{&ptr} {}

    explicit DestroyCount(int &ptr, int value) : ptr{&ptr}, value_{value} {}

    DestroyCount(const DestroyCount &) = delete;

    DestroyCount(DestroyCount &&other) : ptr{other.ptr}, value_{other.value_} {
        other.ptr = nullptr;
    }

    DestroyCount &operator=(const DestroyCount &) = delete;

    DestroyCount &operator=(DestroyCount &&other) {
        ptr = other.ptr;
        value_ = other.value_;
        other.ptr = nullptr;
        return *this;
    }

    ~DestroyCount() {
        if (ptr != nullptr) {
            ++*ptr;
        }
    }
};

struct MoveOnly {
    int value;

    explicit MoveOnly(int value) : value{value} {}

    MoveOnly(const MoveOnly &) = delete;

    MoveOnly(MoveOnly &&other) : value{other.value} {
        other.value = -1;
    }

    MoveOnly &operator=(const MoveOnly &) = delete;

    MoveOnly &operator=(MoveOnly &&other) {
        value = other.value;
        other.value = -1;
        return *this;
    };

    inline bool is_moved() const {
        return value == -1;
    }
};
