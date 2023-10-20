#pragma once

#include "./pair_ptr.h"
#include <etl/functional.h>

namespace memory {
    namespace private_pair_shared {
        template <typename T>
        class OwnedInner;

        template <typename T>
        class ReferenceInner;

        template <typename T>
        class OwnedInner : public memory::PairPtr<OwnedInner<T>, ReferenceInner<T>> {
            T value_;

          public:
            inline constexpr OwnedInner() = delete;
            inline constexpr OwnedInner(const OwnedInner &) = delete;
            inline constexpr OwnedInner(OwnedInner &&) = default;
            inline constexpr OwnedInner &operator=(const OwnedInner &) = delete;
            inline constexpr OwnedInner &operator=(OwnedInner &&) = default;

            inline constexpr OwnedInner(ReferenceInner<T> *reference, T &&value)
                : memory::PairPtr<OwnedInner<T>, ReferenceInner<T>>{reference},
                  value_{etl::move(value)} {}

            inline constexpr static OwnedInner<T> dangling(T &&value) {
                return OwnedInner<T>{nullptr, etl::move(value)};
            }

            inline constexpr T &get() {
                return value_;
            }

            inline constexpr const T &get() const {
                return value_;
            }

            inline constexpr etl::optional<ReferenceInner<T>> try_create_pair() {
                if (this->has_pair()) {
                    return etl::nullopt;
                }
                ReferenceInner<T> reference{this};
                this->unsafe_set_pair(&reference);
                return etl::move(reference);
            }
        };

        template <typename T>
        class ReferenceInner : public memory::PairPtr<ReferenceInner<T>, OwnedInner<T>> {
          public:
            inline constexpr ReferenceInner() = delete;
            inline constexpr ReferenceInner(const ReferenceInner &) = delete;
            inline constexpr ReferenceInner(ReferenceInner &&) = default;
            inline constexpr ReferenceInner &operator=(const ReferenceInner &) = delete;
            inline constexpr ReferenceInner &operator=(ReferenceInner &&) = default;

            explicit inline constexpr ReferenceInner(OwnedInner<T> *owned)
                : memory::PairPtr<ReferenceInner<T>, OwnedInner<T>>{owned} {}

            inline constexpr static ReferenceInner<T> dangling() {
                return ReferenceInner<T>{nullptr};
            }

            inline constexpr etl::optional<etl::reference_wrapper<T>> get() {
                auto pair = this->get_pair();
                if (pair.has_value()) {
                    return etl::ref(pair->get().get());
                }
                return etl::nullopt;
            }

            inline constexpr etl::optional<etl::reference_wrapper<const T>> get() const {
                const auto pair = this->get_pair();
                if (pair.has_value()) {
                    return etl::cref(pair->get().get());
                }
                return etl::nullopt;
            }
        };

        template <typename T>
        etl::pair<OwnedInner<T>, ReferenceInner<T>> make_shared_inner(T &&value) {
            OwnedInner<T> owned{nullptr, etl::move(value)};
            ReferenceInner<T> reference{&owned};
            owned.unsafe_set_pair(&reference);
            return {etl::move(owned), etl::move(reference)};
        }
    } // namespace private_pair_shared

    template <typename T>
    class Reference;

    template <typename T>
    class Owned {
        private_pair_shared::OwnedInner<T> inner_;

      public:
        inline constexpr Owned() = delete;
        inline constexpr Owned(const Owned &) = delete;
        inline constexpr Owned(Owned &&) = default;
        inline constexpr Owned &operator=(const Owned &) = delete;
        inline constexpr Owned &operator=(Owned &&) = default;

        inline constexpr Owned(private_pair_shared::OwnedInner<T> &&inner)
            : inner_{etl::move(inner)} {}

        inline constexpr Owned(T &&value) : inner_{nullptr, etl::move(value)} {}

        inline constexpr T &get() {
            return inner_.get();
        }

        inline constexpr const T &get() const {
            return inner_.get();
        }

        inline constexpr bool has_pair() const {
            return inner_.has_pair();
        }

        inline constexpr void unpair() {
            inner_.unpair();
        }

        inline constexpr etl::optional<Reference<T>> try_create_pair() {
            auto pair = inner_.try_create_pair();
            if (pair.has_value()) {
                return Reference<T>{etl::move(pair.value())};
            } else {
                return etl::nullopt;
            }
        }
    };

    template <typename T>
    class Reference {
        private_pair_shared::ReferenceInner<T> inner_;

      public:
        inline constexpr Reference() = delete;
        inline constexpr Reference(const Reference &) = delete;
        inline constexpr Reference(Reference &&) = default;
        inline constexpr Reference &operator=(const Reference &) = delete;
        inline constexpr Reference &operator=(Reference &&) = default;

        inline constexpr Reference(private_pair_shared::ReferenceInner<T> &&inner)
            : inner_{etl::move(inner)} {}

        inline constexpr static Reference<T> dangling() {
            return Reference<T>{private_pair_shared::ReferenceInner<T>::dangling()};
        }

        inline constexpr etl::optional<etl::reference_wrapper<T>> get() {
            return inner_.get();
        }

        inline constexpr etl::optional<etl::reference_wrapper<const T>> get() const {
            return inner_.get();
        }

        inline constexpr bool has_pair() const {
            return inner_.has_pair();
        }

        inline constexpr void unpair() {
            inner_.unpair();
        }
    };

    template <typename T>
    etl::pair<Owned<T>, Reference<T>> make_shared(T &&value) {
        auto [owned, reference] = private_pair_shared::make_shared_inner(etl::move(value));
        return {etl::move(owned), etl::move(reference)};
    }
} // namespace memory
