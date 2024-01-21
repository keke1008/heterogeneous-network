#pragma once

#include <etl/functional.h>
#include <etl/optional.h>
#include <etl/utility.h>

namespace memory {
    template <typename Derived, typename Pair>
    class PairPtr {
        friend PairPtr<Pair, Derived>;

        Pair *pair_;

      public:
        PairPtr() = delete;
        PairPtr(const PairPtr &) = delete;
        PairPtr &operator=(const PairPtr &) = delete;

        inline constexpr PairPtr(PairPtr &&other) {
            pair_ = other.pair_;
            if (pair_ != nullptr) {
                pair_->pair_ = static_cast<Derived *>(this);
                other.pair_ = nullptr;
            }
        }

        inline constexpr PairPtr &operator=(PairPtr &&other) {
            if (this == &other) {
                return *this;
            }

            if (pair_ != nullptr) {
                pair_->pair_ = nullptr;
            }
            pair_ = other.pair_;
            if (pair_ != nullptr) {
                pair_->pair_ = static_cast<Derived *>(this);
                other.pair_ = nullptr;
            }
            return *this;
        }

        inline constexpr PairPtr(Pair *pair) : pair_{pair} {}

        ~PairPtr() {
            if (pair_ != nullptr) {
                pair_->pair_ = nullptr;
            }
        }

        inline static constexpr PairPtr dangling() {
            return PairPtr{nullptr};
        }

        inline constexpr void unpair() {
            if (pair_ != nullptr) {
                pair_->pair_ = nullptr;
                pair_ = nullptr;
            }
        }

        inline constexpr etl::optional<etl::reference_wrapper<const Pair>> get_pair() const {
            if (pair_ != nullptr) {
                return etl::cref(*pair_);
            }
            return etl::nullopt;
        }

        inline constexpr etl::optional<etl::reference_wrapper<Pair>> get_pair() {
            if (pair_ != nullptr) {
                return etl::ref(*pair_);
            }
            return etl::nullopt;
        }

        inline constexpr bool has_pair() const {
            return pair_ != nullptr;
        }

        inline constexpr void unsafe_set_pair(Pair *pair) {
            pair_ = pair;
        }

        inline constexpr Pair *unsafe_get_pair() {
            return pair_;
        }

        inline constexpr const Pair *unsafe_get_pair() const {
            return pair_;
        }
    };
} // namespace memory
