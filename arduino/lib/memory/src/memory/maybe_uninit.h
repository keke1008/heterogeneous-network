#pragma once

#include <etl/memory.h>
#include <etl/utility.h>
#include <stdint.h>

namespace memory {
    struct ZeroInit {};

    constexpr ZeroInit zero_init{};

    template <typename T>
    union MaybeUninit {
      private:
        uint8_t empty_[0];
        T value_;

      public:
        MaybeUninit(const MaybeUninit<T> &) = delete;
        MaybeUninit(MaybeUninit<T> &&) = delete;
        MaybeUninit &operator=(const MaybeUninit<T> &) = delete;
        MaybeUninit &operator=(MaybeUninit<T> &&) = delete;

        inline MaybeUninit() {}

        inline MaybeUninit(ZeroInit) {
            etl::memory_set<T>(value_, 0);
        }

        inline MaybeUninit(const T &value) : value_{value} {}

        inline MaybeUninit(T &&value) : value_{etl::move(value)} {}

        inline ~MaybeUninit() {}

        inline T &get() {
            return value_;
        }

        inline const T &get() const {
            return value_;
        }

        inline T *get_ptr() {
            return &value_;
        }

        inline const T *get_ptr() const {
            return &value_;
        }

        inline void set(const T &value) {
            get() = value; // Copy constructorが無い場合にコンパイルエラーにするため
        }

        inline void set(T &&value) {
            get() = etl::move(value); // Move constructorが無い場合にコンパイルエラーにするため
        }

        inline void destroy() {
            value_.~T();
        }

        inline void destroy_and_set(const T &value) {
            destroy();
            set(value);
        }

        inline void destroy_and_set(T &&value) {
            destroy();
            set(etl::move(value));
        }
    };
} // namespace memory
