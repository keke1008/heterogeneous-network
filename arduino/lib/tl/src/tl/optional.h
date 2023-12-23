#pragma once

#include <etl/utility.h>
#include <logger.h>
#include <stdint.h>

namespace tl {
    namespace optional {
        template <typename T>
        union Storage {
          private:
            T value_;
            uint8_t dummy_[0];

          public:
            Storage &operator=(const Storage &other) = delete;
            Storage &operator=(Storage &&other) = delete;

            Storage() : dummy_{} {}

            ~Storage() {}

            T &get() {
                return value_;
            }

            const T &get() const {
                return value_;
            }

            T *as_ptr() {
                return &value_;
            }

            const T *as_ptr() const {
                return &value_;
            }
        };
    } // namespace optional

    struct Nullopt {
        explicit constexpr Nullopt(int) {}
    };

    constexpr Nullopt nullopt{0};

    template <
        typename T,
        bool Movable = etl::is_move_constructible_v<T>,
        bool Copyable = etl::is_copy_constructible_v<T>>
    class Optional;

    template <typename T>
    class Optional<T, false, false> {
      protected:
        bool has_value_{false};
        optional::Storage<T> storage_;

        void panic_if_no_value() const {
            if (!has_value_) {
                PANIC("Optional empty");
            }
        }

      public:
        Optional(const Optional &) = delete;
        Optional &operator=(const Optional &) = delete;
        Optional(Optional &&) = delete;
        Optional &operator=(Optional &&) = delete;

        ~Optional() {
            if (has_value_) {
                storage_.get().~T();
            }
        }

        Optional() : storage_{} {};
        Optional(Nullopt) : storage_{} {};

        template <
            typename... Args,
            typename = etl::enable_if_t<etl::is_constructible_v<T, Args...>>>
        Optional(Args &&...args) : has_value_{true} {
            new (storage_.as_ptr()) T(etl::forward<Args>(args)...);
        }

        Optional &operator=(Nullopt) {
            if (has_value_) {
                storage_.get().~T();
                has_value_ = false;
            }

            return *this;
        }

        template <typename... Args>
        inline auto emplace(Args &&...args) {
            if (has_value_) {
                storage_.get().~T();
            }

            new (storage_.as_ptr()) T(etl::forward<Args>(args)...);
            has_value_ = true;
        }

        inline void reset() {
            if (has_value_) {
                storage_.get().~T();
                has_value_ = false;
            }
        }

        inline constexpr T &operator*() & {
            return storage_.get();
        }

        inline constexpr const T &operator*() const & {
            return storage_.get();
        }

        inline constexpr T &&operator*() && {
            return etl::move(storage_.get());
        }

        inline constexpr const T &&operator*() const && {
            return etl::move(storage_.get());
        }

        inline constexpr T *operator->() {
            return storage_.as_ptr();
        }

        inline constexpr const T *operator->() const {
            return storage_.as_ptr();
        }

        inline constexpr explicit operator bool() const {
            return has_value_;
        }

        inline constexpr bool has_value() const {
            return has_value_;
        }

        inline constexpr T &value() & {
            panic_if_no_value();
            return storage_.get();
        }

        inline constexpr const T &value() const & {
            panic_if_no_value();
            return storage_.get();
        }

        inline constexpr T &&value() && {
            panic_if_no_value();
            return etl::move(storage_.get());
        }

        inline constexpr const T &&value() const && {
            panic_if_no_value();
            return etl::move(storage_.get());
        }

        inline constexpr bool operator==(const Optional &other) const {
            if (has_value_ != other.has_value_) {
                return false;
            } else if (has_value_) {
                return storage_.get() == other.storage_.get();
            } else {
                return true;
            }
        }

        inline constexpr bool operator!=(const Optional &other) const {
            return !(*this == other);
        }

        inline constexpr bool operator<(const Optional &other) const {
            if (has_value_ != other.has_value_) {
                return !has_value_;
            } else if (has_value_) {
                return storage_.get() < other.storage_.get();
            } else {
                return false;
            }
        }

        inline constexpr bool operator>(const Optional &other) const {
            if (has_value_ != other.has_value_) {
                return has_value_;
            } else if (has_value_) {
                return storage_.get() > other.storage_.get();
            } else {
                return false;
            }
        }

        inline constexpr bool operator<=(const Optional &other) const {
            return !(*this > other);
        }

        inline constexpr bool operator>=(const Optional &other) const {
            return !(*this < other);
        }
    };

    template <typename T>
    class Optional<T, true, false> : public Optional<T, false, false> {
      protected:
        using Base = Optional<T, false, false>;
        using Base::has_value_;
        using Base::storage_;

      public:
        using Base::Base;
        Optional(const Optional &) = delete;
        Optional &operator=(const Optional &) = delete;

        Optional(Optional &&other) {
            has_value_ = other.has_value_;
            if (has_value_) {
                new (storage_.as_ptr()) T(etl::move(other.storage_.get()));
            }
            other.has_value_ = false;
        }

        Optional &operator=(Optional &&other) {
            if (has_value_) {
                if (other.has_value_) {
                    storage_.get() = etl::move(other.storage_.get());
                } else {
                    storage_.get().~T();
                }
            } else {
                if (other.has_value_) {
                    new (storage_.as_ptr()) T(etl::move(other.storage_.get()));
                } else {
                    // do nothing
                }
            }

            has_value_ = other.has_value_;
            other.has_value_ = false;
            return *this;
        }

        Optional(T &&value) {
            has_value_ = true;
            new (storage_.as_ptr()) T(etl::move(value));
        };

        Optional &operator=(T &&value) {
            if (has_value_) {
                storage_.get() = etl::move(value);
            } else {
                new (storage_.as_ptr()) T(etl::move(value));
                has_value_ = true;
            }

            return *this;
        }

        template <typename U>
        inline constexpr T value_or(U &&default_value) const & {
            if (has_value_) {
                return storage_.get();
            } else {
                return etl::forward<U>(default_value);
            }
        }

        template <typename U>
        inline constexpr T value_or(U &&default_value) && {
            if (has_value_) {
                return etl::move(storage_.get());
            } else {
                return etl::forward<U>(default_value);
            }
        }
    };

    template <typename T>
    class Optional<T, true, true> : public Optional<T, true, false> {
        using Base = Optional<T, true, false>;
        using Base::has_value_;
        using Base::storage_;

      public:
        using Base::Base;

        Optional(const Optional &other) {
            has_value_ = other.has_value_;
            if (has_value_) {
                new (storage_.as_ptr()) T(other.storage_.get());
            }
        }

        Optional &operator=(const Optional &other) {
            if (has_value_) {
                if (other.has_value_) {
                    storage_.get() = other.storage_.get();
                } else {
                    storage_.get().~T();
                }
            } else {
                if (other.has_value_) {
                    new (storage_.as_ptr()) T(other.storage_.get());
                } else {
                    // do nothing
                }
            }

            has_value_ = other.has_value_;
            return *this;
        }

        Optional(const T &value) {
            has_value_ = true;
            new (storage_.as_ptr()) T(value);
        };

        Optional &operator=(const T &value) {
            if (has_value_) {
                storage_.get() = value;
            } else {
                new (storage_.as_ptr()) T(value);
                has_value_ = true;
            }

            return *this;
        }
    };

    template <typename T>
    Optional(T) -> Optional<T>;

    template <typename T>
    inline constexpr Optional<T> make_optional(T &&value) {
        return Optional<T>(etl::forward<T>(value));
    }

    template <typename T, typename... Args>
    inline constexpr Optional<T> make_optional(Args &&...args) {
        return Optional<T>(etl::forward<Args>(args)...);
    }
} // namespace tl
