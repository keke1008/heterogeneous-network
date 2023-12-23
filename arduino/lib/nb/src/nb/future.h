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
        class FutureInner final : public memory::PairPtr<FutureInner<T>, PromiseInner<T>> {
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

            inline constexpr bool never_receive_value() const {
                return !this->has_pair() && !value_.has_value();
            }
        };

        template <>
        class FutureInner<void> final
            : public memory::PairPtr<FutureInner<void>, PromiseInner<void>> {
            bool has_value_{false};

          public:
            FutureInner() = delete;
            FutureInner(const FutureInner &) = delete;
            inline FutureInner(FutureInner &&) = default;
            FutureInner &operator=(const FutureInner &) = delete;
            inline FutureInner &operator=(FutureInner &&) = default;

            inline FutureInner(PromiseInner<void> *promise)
                : memory::PairPtr<FutureInner<void>, PromiseInner<void>>{promise} {}

            inline constexpr void set() {
                has_value_ = true;
            }

            inline nb::Poll<void> poll() {
                if (has_value_) {
                    return nb::ready();
                }
                return nb::pending;
            }

            inline constexpr bool is_closed() const {
                return !this->has_pair();
            }
        };

        template <typename T>
        class PromiseInner final : public memory::PairPtr<PromiseInner<T>, FutureInner<T>> {
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

        template <>
        class PromiseInner<void> final
            : public memory::PairPtr<PromiseInner<void>, FutureInner<void>> {
          public:
            PromiseInner() = delete;
            PromiseInner(const PromiseInner &) = delete;
            PromiseInner(PromiseInner &&) = default;
            PromiseInner &operator=(const PromiseInner &) = delete;
            PromiseInner &operator=(PromiseInner &&) = default;

            inline constexpr PromiseInner(FutureInner<void> *future)
                : memory::PairPtr<PromiseInner<void>, FutureInner<void>>{future} {}

            inline void set_value() {
                auto pair = this->get_pair();
                if (pair.has_value()) {
                    pair->get().set();
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

        template <>
        inline etl::pair<FutureInner<void>, PromiseInner<void>>
        make_future_promise_pair_inner<void>() {
            FutureInner<void> future{nullptr};
            PromiseInner<void> promise{&future};
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

        inline constexpr nb::Poll<
            etl::conditional_t<etl::is_void_v<T>, void, etl::reference_wrapper<T>>>
        poll() {
            return inner_.poll();
        }

        inline constexpr bool is_closed() const {
            return inner_.is_closed();
        }

        inline constexpr bool never_receive_value() const {
            return inner_.never_receive_value();
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

    template <>
    class Promise<void> {
        private_future::PromiseInner<void> inner_;

      public:
        Promise() = delete;
        Promise(const Promise &) = delete;
        Promise(Promise &&) = default;
        Promise &operator=(const Promise &) = delete;
        Promise &operator=(Promise &&) = default;

        inline Promise(private_future::PromiseInner<void> &&inner) : inner_{etl::move(inner)} {}

        inline void set_value() {
            inner_.set_value();
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
