#pragma once

#include "./result.h"
#include <etl/utility.h>
#include <etl/variant.h>

namespace nb {
    class Pending {
      public:
        constexpr inline bool operator==(const Pending &) const {
            return true;
        }

        constexpr inline bool operator!=(const Pending &) const {
            return false;
        }
    };

    template <typename T>
    class Ready {
        T value_;

      public:
        constexpr inline Ready(const T &value) : value_{value} {}

        constexpr inline Ready(T &&value) : value_{etl::move(value)} {}

        constexpr inline bool operator==(const Ready &other) const {
            return value_ == other.value_;
        }

        constexpr inline bool operator!=(const Ready &other) const {
            return value_ != other.value_;
        }

        constexpr inline const T &get() const {
            return value_;
        }

        constexpr inline T &get() {
            return value_;
        }
    };

    template <typename T>
    class Poll {
        etl::variant<Pending, Ready<T>> value_;

      public:
        Poll() = delete;

        Poll(const Ready<T> &other) : value_{other} {}

        Poll(Ready<T> &&other) : value_{etl::move(other)} {}

        Poll(const Pending &other) : value_{other} {}

        Poll(Pending &&other) : value_{etl::move(other)} {}

        template <typename... Ts>
        Poll(Ts &&...ts) : value_{Ready<T>{etl::forward<Ts>(ts)...}} {}

        constexpr inline bool operator==(const Poll &other) const {
            if (is_pending()) {
                return other.is_pending();
            }
            return other.is_ready() && unwrap() == other.unwrap();
        }

        constexpr inline bool operator!=(const Poll &other) const {
            return !(*this == other);
        }

        constexpr inline bool is_pending() const {
            return etl::holds_alternative<Pending>(value_);
        }

        constexpr inline bool is_ready() const {
            return etl::holds_alternative<Ready<T>>(value_);
        }

        constexpr inline const T &unwrap() const {
            return etl::get<Ready<T>>(value_).get();
        }

        constexpr inline T &unwrap() {
            return etl::get<Ready<T>>(value_).get();
        }

        constexpr inline const T &unwrap_or(const T &default_value) const {
            if (is_ready()) {
                return unwrap();
            }
            return default_value;
        }

        constexpr inline T &unwrap_or(T &default_value) {
            if (is_ready()) {
                return unwrap();
            }
            return default_value;
        }

        constexpr inline const T &unwrap_or_default() const {
            T default_value{};
            return unwrap_or(default_value);
        }

        constexpr inline T &unwrap_or_default() {
            T default_value{};
            return unwrap_or(default_value);
        }

        template <typename F>
        constexpr inline const auto map(F &&f) const {
            if (is_ready()) {
                return Poll{Ready{f(unwrap())}};
            }
            return Poll{Pending{}};
        }

        template <typename F>
        constexpr inline auto map(F &&f) {
            if (is_ready()) {
                return Poll{Ready{f(unwrap())}};
            }
            return Poll{Pending{}};
        }

        template <typename F>
        constexpr inline const auto and_then(F &&f) const {
            if (is_ready()) {
                return f(unwrap());
            }
            return Poll{Pending{}};
        }

        template <typename F>
        constexpr inline auto bind_ready(F &&f) {
            if (is_ready()) {
                return f(unwrap());
            }
            return Poll{Pending{}};
        }

        template <typename F>
        constexpr inline const auto bind_pending(F &&f) const {
            if (is_pending()) {
                return f();
            }
            return *this;
        }

        template <typename F>
        constexpr inline auto bind_pending(F &&f) {
            if (is_pending()) {
                return f();
            }
            return *this;
        }
    };

    const inline Pending pending{};

#define POLL_UNWRAP_OR_RETURN(value)                                                               \
    ({                                                                                             \
        decltype(auto) v = value;                                                                  \
        if (v.is_pending()) {                                                                      \
            return nb::pending;                                                                    \
        }                                                                                          \
        v.unwrap();                                                                                \
    })
} // namespace nb
