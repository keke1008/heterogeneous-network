#pragma once

#include "./poll.h"
#include <etl/functional.h>
#include <etl/optional.h>
#include <etl/type_traits.h>
#include <etl/utility.h>
#include <memory/pair_ptr.h>

namespace nb {
    namespace private_future {
        template <typename T>
        class FutureInner;

        template <typename T>
        class PromiseInner;

        template <typename T>
        class FutureInner : public memory::PairPtr<FutureInner<T>, PromiseInner<T>> {
            etl::optional<T> value_{etl::nullopt};

          public:
            FutureInner() = delete;
            FutureInner(const FutureInner &) = delete;
            inline constexpr FutureInner(FutureInner &&) = default;
            FutureInner &operator=(const FutureInner &) = delete;
            inline constexpr FutureInner &operator=(FutureInner &&) = default;

            inline constexpr FutureInner(PromiseInner<T> *promise)
                : memory::PairPtr<FutureInner<T>, PromiseInner<T>>{promise} {}

            inline constexpr void set(const T &value) {
                value_ = value;
            }

            inline constexpr void set(T &&value) {
                value_ = etl::move(value);
            }

            inline constexpr nb::Poll<T &&> get() {
                if (value_.has_value()) {
                    return etl::move(*value_);
                }
                return nb::pending;
            }

            inline constexpr nb::Poll<etl::reference_wrapper<T>> poll() {
                if (value_.has_value()) {
                    return etl::ref(value_.value());
                }
                return nb::pending;
            }

            inline constexpr bool is_closed() const {
                return !this->has_pair();
            }
        };

        template <typename T>
        class PromiseInner : public memory::PairPtr<PromiseInner<T>, FutureInner<T>> {
          public:
            inline constexpr PromiseInner() = delete;
            inline constexpr PromiseInner(const PromiseInner &) = delete;
            inline constexpr PromiseInner(PromiseInner &&) = default;
            inline constexpr PromiseInner &operator=(const PromiseInner &) = delete;
            inline constexpr PromiseInner &operator=(PromiseInner &&) = default;

            inline constexpr PromiseInner(FutureInner<T> *future)
                : memory::PairPtr<PromiseInner<T>, FutureInner<T>>{future} {}

            inline constexpr void set_value(const T &value) {
                auto pair = this->get_pair();
                if (pair.has_value()) {
                    pair->get().set(value);
                }
            }

            inline constexpr void set_value(T &&value) {
                auto pair = this->get_pair();
                if (pair.has_value()) {
                    pair->get().set(etl::move(value));
                }
            }

            inline constexpr bool is_closed() const {
                return !this->has_pair();
            }
        };

        template <typename T>
        etl::pair<FutureInner<T>, PromiseInner<T>> make_future_promise_pair_inner() {
            FutureInner<T> future{nullptr};
            PromiseInner<T> promise{&future};
            future.unsafe_set_pair(&promise);
            return {etl::move(future), etl::move(promise)};
        }
    } // namespace private_future

    template <typename T>
    class Future {
        private_future::FutureInner<T> inner_;

      public:
        inline constexpr Future() = delete;
        inline constexpr Future(const Future &) = delete;
        inline constexpr Future(Future &&) = default;
        inline constexpr Future &operator=(const Future &) = delete;
        inline constexpr Future &operator=(Future &&) = default;

        inline constexpr Future(private_future::FutureInner<T> &&inner)
            : inner_{etl::move(inner)} {}

        inline constexpr nb::Poll<etl::reference_wrapper<T>> poll() {
            return inner_.poll();
        }

        inline constexpr bool is_closed() const {
            return inner_.is_closed();
        }
    };

    template <typename T>
    class Promise {
        private_future::PromiseInner<T> inner_;

      public:
        inline constexpr Promise() = delete;
        inline constexpr Promise(const Promise &) = delete;
        inline constexpr Promise(Promise &&) = default;
        inline constexpr Promise &operator=(const Promise &) = delete;
        inline constexpr Promise &operator=(Promise &&) = default;

        inline constexpr Promise(private_future::PromiseInner<T> &&inner)
            : inner_{etl::move(inner)} {}

        inline constexpr void set_value(const T &value) {
            inner_.set_value(value);
        }

        inline constexpr void set_value(T &&value) {
            inner_.set_value(etl::move(value));
        }

        inline constexpr bool is_closed() const {
            return inner_.is_closed();
        }
    };

    template <typename T>
    etl::pair<Future<T>, Promise<T>> make_future_promise_pair() {
        auto [future, promise] = private_future::make_future_promise_pair_inner<T>();
        return {Future<T>{etl::move(future)}, Promise<T>{etl::move(promise)}};
    }
}; // namespace nb
