#pragma once

#include <etl/span.h>
#include <etl/utility.h>
#include <etl/vector.h>
#include <logger.h>

namespace memory {
    template <typename T>
    class StaticRef;

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

        ~Static() {
            LOG_ERROR("Static object is destructed");
        }

        inline T &operator*() {
            return value_;
        }

        inline const T &operator*() const {
            return value_;
        }

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

        template <typename U = T>
        inline StaticRef<U> ref() {
            return StaticRef<U>{static_cast<U &>(value_)};
        }

        template <typename U = T>
        inline StaticRef<const U> ref() const {
            return StaticRef<U>{static_cast<const U &>(value_)};
        }

        template <typename U = T>
        inline StaticRef<const U> cref() const {
            return StaticRef<U>{static_cast<const U &>(value_)};
        }
    };

    /**
     * `StaticRef`は`Static`の内部の値への参照を提供する．
     *
     * 通常は`Static<T>&`を使用すれば問題ないが，`T`を抽象クラスとして扱いたい場合はこのクラスを使用する．
     */
    template <typename T>
    class StaticRef {
        template <typename U>
        friend class Static;

        T *value_;

        explicit StaticRef(T &value) : value_{&value} {}

      public:
        StaticRef() = delete;
        StaticRef(const StaticRef &) = default;
        StaticRef(StaticRef &&) = default;
        StaticRef &operator=(const StaticRef &) = default;
        StaticRef &operator=(StaticRef &&) = default;

        inline T &operator*() {
            return *value_;
        }

        inline const T &operator*() const {
            return *value_;
        }

        inline T *operator->() {
            return value_;
        }

        inline const T *operator->() const {
            return value_;
        }

        inline T &get() {
            return *value_;
        }

        inline const T &get() const {
            return *value_;
        }
    };
} // namespace memory
