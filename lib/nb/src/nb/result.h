#pragma once

#include <etl/optional.h>
#include <etl/utility.h>
#include <etl/variant.h>
#include <util/type_traits.h>
#include <util/visitor.h>

namespace nb {
    template <typename T, typename E>
    class Result;

    class Empty {
      public:
        bool operator==(const Empty &) {
            return true;
        }

        bool operator!=(const Empty &) {
            return false;
        }
    };

    const inline Empty empty{};

    template <typename T>
    class Ok {
        T value_;

      public:
        Ok() = default;

        Ok(const T &value) : value_{value} {}

        Ok(T &&value) : value_{etl::move(value)} {}

        template <typename E>
        operator Result<T, E>() {
            return Result<T, E>{*this};
        }

        bool operator==(const Ok &other) {
            return value_ == other.value_;
        }

        bool operator!=(const Ok &other) {
            return value_ != other.value_;
        }

        T &operator*() {
            return value_;
        }

        const T &operator*() const {
            return value_;
        }

        T *operator->() {
            return &value_;
        }

        const T *operator->() const {
            return &value_;
        }
    };

    template <typename E>
    class Err {
        E value_;

      public:
        Err() = default;

        Err(const E &value) : value_{value} {}

        Err(E &&value) : value_{etl::move(value)} {}

        template <typename T>
        operator Result<T, E>() {
            return Result<T, E>{*this};
        }

        bool operator==(const Err &other) {
            return value_ == other.value_;
        }

        bool operator!=(const Err &other) {
            return value_ != other.value_;
        }

        E &operator*() & {
            return value_;
        }

        const E &operator*() const & {
            return value_;
        }

        E *operator->() & {
            return &value_;
        }

        const E *operator->() const & {
            return &value_;
        }
    };

    template <typename T, typename E>
    class Result {
        etl::variant<Ok<T>, Err<E>> state_;

        using OkType = T;
        using ErrType = E;

      public:
        Result() = delete;
        Result(const Result &) = default;
        Result(Result &&) = default;
        Result &operator=(const Result &) = default;
        Result &operator=(Result &&) = default;

        Result(const Ok<T> &ok) : state_{ok} {}

        Result(Ok<T> &&ok) : state_{etl::move(ok)} {}

        Result(const Err<E> &err) : state_{err} {}

        Result(Err<E> &&err) : state_{etl::move(err)} {}

        inline bool operator==(const Result &other) const {
            if (is_ok() && other.is_ok()) {
                return unwrap_ok() == other.unwrap_ok();
            } else if (is_err() && other.is_err()) {
                return unwrap_err() == other.unwrap_err();
            } else {
                return false;
            }
        }

        inline bool operator!=(const Result &other) const {
            return !(*this == other);
        }

        inline bool is_ok() const {
            return etl::holds_alternative<Ok<T>>(state_);
        }

        inline bool is_err() const {
            return etl::holds_alternative<Err<E>>(state_);
        }

        inline T &unwrap_ok() {
            return *etl::get<Ok<T>>(state_);
        }

        inline const T &unwrap_ok() const {
            return *etl::get<Ok<T>>(state_);
        }

        inline T &unwrap_ok_or(T &default_value) {
            if (is_ok()) {
                return unwrap_ok();
            }
            return default_value;
        }

        inline const T &unwrap_ok_or(const T &default_value) const {
            if (is_ok()) {
                return unwrap_ok();
            }
            return default_value;
        }

        inline T unwrap_ok_or(T &&default_value) {
            if (is_ok()) {
                return etl::move(unwrap_ok());
            }
            return etl::move(default_value);
        }

        inline E &unwrap_err() {
            return *etl::get<Err<E>>(state_);
        }

        inline const E &unwrap_err() const {
            return *etl::get<Err<E>>(state_);
        }

        inline E &unwrap_err_or(E &default_value) {
            if (is_err()) {
                return unwrap_err();
            }
            return default_value;
        }

        inline const E &unwrap_err_or(const E &default_value) const {
            if (is_err()) {
                return unwrap_err();
            }
            return default_value;
        }

        inline E unwrap_err_or(E &&default_value) {
            if (is_err()) {
                return etl::move(unwrap_err());
            }
            return etl::move(default_value);
        }

        template <typename U, typename V>
        inline U visit(V &&visitor) {
            return etl::visit<U>(etl::forward<V>(visitor), state_);
        }

        template <typename U = T, typename F>
        inline Result<U, E> bind_ok(F &&f) {
            return visit<Result<U, E>>(util::Visitor{
                [&](Ok<T> &ok) { return f(*ok); },
                [&](Err<E> &err) { return Err{*err}; },
            });
        }

        template <typename U, typename F>
        inline Result<T, U> bind_err(F &&f) {
            return visit<Result<T, U>>(util::Visitor{
                [&](Ok<T> &ok) { return Ok{*ok}; },
                [&](Err<E> &err) { return f(*err); },
            });
        }

        template <typename F>
        inline Result<util::invoke_result_t<F, T>, E> map_ok(F &&f) {
            return bind_ok<util::invoke_result_t<F, T>>([&](T &ok) { return Ok{f(ok)}; });
        }

        template <typename F>
        inline Result<T, util::invoke_result_t<F, E>> map_err(F &&f) {
            return bind_err<util::invoke_result_t<F, E>>([&](E &err) { return Err{f(err)}; });
        }

        template <typename U = T, typename F>
        inline U map_ok_or(const U &default_value, F &&f) {
            return visit<U>(util::Visitor{
                [&](Ok<T> &ok) { return f(*ok); },
                [&](auto &) { return default_value; },
            });
        }

        template <typename U = E, typename F>
        inline U map_err_or(const U &default_value, F &&f) {
            return visit<U>(util::Visitor{
                [&](Err<E> &err) { return f(*err); },
                [&](auto &) { return default_value; },
            });
        }
    };
} // namespace nb
