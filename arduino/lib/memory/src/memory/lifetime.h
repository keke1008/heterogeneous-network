#pragma once

#include <etl/utility.h>

namespace memory {
    template <typename T>
    class Static;

    template <typename T>
    class StaticRef {
        friend class Static<T>;

        T *value_;

        inline StaticRef(T &value) : value_{value} {}

      public:
        StaticRef() = delete;
        StaticRef(const StaticRef &) = default;
        StaticRef(StaticRef &&) = default;
        StaticRef &operator=(const StaticRef &) = default;
        StaticRef &operator=(StaticRef &&) = default;

        inline T &get() {
            return *value_;
        }

        inline const T &get() const {
            return *value_;
        }

        inline T *operator->() {
            return value_;
        }

        inline const T *operator->() const {
            return value_;
        }
    };

    template <typename T>
    class Static {
        T value_;

      public:
        Static() = delete;
        Static(const Static &) = delete;
        Static(Static &&) = delete;
        Static &operator=(const Static &) = delete;
        Static &operator=(Static &&) = delete;

        inline Static(T &&value) : value_{etl::move(value)} {}

        inline Static(const T &value) : value_{value} {}

        inline StaticRef<T> ref() {
            return StaticRef<T>{value_};
        }

        inline StaticRef<const T> ref() const {
            return StaticRef<const T>{value_};
        }

        inline StaticRef<T> cref() const {
            return StaticRef<T>{value_};
        }

        inline T &get() {
            return value_;
        }

        inline const T &get() const {
            return value_;
        }
    };

} // namespace memory
