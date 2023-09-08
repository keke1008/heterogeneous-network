#pragma once

#include "./result.h"
#include <etl/optional.h>
#include <etl/utility.h>

namespace nb {
    class Pending {};

    template <typename T>
    class Ready {
        T value_;

      public:
        constexpr inline Ready(const T &value) : value_{value} {}

        constexpr inline Ready(T &&value) : value_{etl::move(value)} {}

        constexpr inline const T &get() const {
            return value_;
        }

        constexpr inline T &get() {
            return value_;
        }
    };

    template <>
    class Ready<void> {
      public:
        constexpr inline Ready() = default;
    };

    Ready() -> Ready<void>;

    template <typename T>
    class Poll {
        etl::optional<T> value_;

      public:
        Poll() = delete;
        Poll(const Poll &) = default;
        Poll(Poll &) = default;
        Poll(Poll &&) = default;
        Poll &operator=(const Poll &) = default;
        Poll &operator=(Poll &&) = default;

        Poll(const Ready<T> &other) : value_{other.get()} {}

        Poll(Ready<T> &&other) : value_{etl::move(other.get())} {}

        Poll(const Pending &other) : value_{etl::nullopt} {}

        Poll(Pending &&other) : value_{etl::nullopt} {}

        template <typename... Ts>
        Poll(Ts &&...ts) : value_{etl::forward<Ts>(ts)...} {}

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
            return !value_.has_value();
        }

        constexpr inline bool is_ready() const {
            return value_.has_value();
        }

        constexpr inline const T &unwrap() const {
            return value_.value();
        }

        constexpr inline T &unwrap() {
            return value_.value();
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
    };

    template <>
    class Poll<void> {
        bool is_ready_;

      public:
        Poll() = delete;
        Poll(const Poll &) = default;
        Poll(Poll &&) = default;
        Poll &operator=(const Poll &) = default;
        Poll &operator=(Poll &&) = default;

        Poll(const Ready<void> &) : is_ready_{true} {}

        Poll(Ready<void> &&) : is_ready_{true} {}

        Poll(const Pending &) : is_ready_{false} {}

        Poll(Pending &&) : is_ready_{false} {}

        constexpr inline bool operator==(const Poll &other) const {
            return is_ready_ == other.is_ready_;
        }

        constexpr inline bool operator!=(const Poll &other) const {
            return !(*this == other);
        }

        constexpr inline bool is_pending() const {
            return !is_ready_;
        }

        constexpr inline bool is_ready() const {
            return is_ready_;
        }

        constexpr inline void unwrap() const {}

        constexpr inline void unwrap_or_default() const {}
    };

    const inline Pending pending{};

    inline Poll<void> ready() {
        return Poll<void>{Ready<void>{}};
    }

    template <typename T>
    inline constexpr Poll<T> ready(T &&t) {
        return Poll<T>{etl::forward<T>(t)};
    }

#define POLL_UNWRAP_OR_RETURN(value)                                                               \
    ({                                                                                             \
        decltype(auto) v = value;                                                                  \
        if (v.is_pending()) {                                                                      \
            return nb::pending;                                                                    \
        }                                                                                          \
        v.unwrap();                                                                                \
    })

#define POLL_RESULT_UNWRAP_OR_RETURN(value)                                                        \
    ({                                                                                             \
        decltype(auto) v_poll = value;                                                             \
        if (v_poll.is_pending()) {                                                                 \
            return nb::pending;                                                                    \
        }                                                                                          \
                                                                                                   \
        decltype(auto) v = v_poll.unwrap();                                                        \
        if (v.is_err()) {                                                                          \
            return nb::Err{v.unwrap_err()};                                                        \
        }                                                                                          \
        v.unwrap_ok();                                                                             \
    })
} // namespace nb
