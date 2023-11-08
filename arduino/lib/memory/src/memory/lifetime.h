#pragma once

#include <etl/span.h>
#include <etl/utility.h>
#include <etl/vector.h>

namespace memory {
    template <typename T>
    class Static {
        T value_;

      public:
        Static() = default;
        Static(const Static &) = delete;
        Static(Static &&) = delete;
        Static &operator=(const Static &) = delete;
        Static &operator=(Static &&) = delete;

        inline Static(T &&value) : value_{etl::move(value)} {}

        inline Static(const T &value) : value_{value} {}

        template <typename... Args>
        inline Static(Args &&...args) : value_{etl::forward<Args>(args)...} {}

        inline T *operator->() {
            return &value_;
        }

        inline const T *operator->() const {
            return &value_;
        }

        inline T &get() {
            return value_;
        }

        inline const T &get() const {
            return value_;
        }
    };
} // namespace memory
