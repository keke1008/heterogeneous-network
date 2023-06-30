#pragma once

#include <etl/utility.h>
#include <stdint.h>

namespace memory {
    template <typename T>
    class Inner {
        uint8_t owner_count_ = 1;
        T value_;

      public:
        Inner() = default;
        Inner(const Inner &) = delete;
        Inner &operator=(const Inner &) = delete;
        Inner(Inner &&other) = delete;
        Inner &operator=(Inner &&other) = delete;

        Inner(T &&value) : value_{etl::move(value)} {}

        inline T *operator->() {
            return &value_;
        }

        inline T &operator*() {
            return value_;
        }

        inline void increment_count() {
            owner_count_++;
        }

        inline void decrement_count() {
            owner_count_--;
        }

        inline bool is_empty() const {
            return owner_count_ == 0;
        }

        inline bool is_unique() const {
            return owner_count_ == 1;
        }

        inline uint8_t ref_count() {
            return owner_count_;
        }
    };
} // namespace memory
