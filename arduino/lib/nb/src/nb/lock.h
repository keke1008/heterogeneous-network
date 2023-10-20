#pragma once

#include <debug_assert.h>
#include <etl/optional.h>
#include <memory/pair_shared.h>

namespace nb {
    template <typename T>
    class LockGuard {
        memory::Reference<T> ref_;

      public:
        LockGuard() = delete;
        LockGuard(const LockGuard &) = delete;
        LockGuard(LockGuard &&) = default;
        LockGuard &operator=(const LockGuard &) = delete;
        LockGuard &operator=(LockGuard &&) = default;

        explicit inline LockGuard(memory::Reference<T> &&value) : ref_{etl::move(value)} {}

        inline T &get() {
            DEBUG_ASSERT(ref_.has_pair());
            return ref_.get();
        }

        inline const T &get() const {
            DEBUG_ASSERT(ref_.has_pair());
            return ref_.get();
        }

        inline void unlock() {
            ref_.unpair();
        }
    };

    template <typename T>
    class Lock {
        memory::Owned<T> value_;

      public:
        Lock() = delete;
        Lock(const Lock &) = delete;
        Lock(Lock &&) = default;
        Lock &operator=(const Lock &) = delete;
        Lock &operator=(Lock &&) = default;

        ~Lock() {
            DEBUG_ASSERT(!value_.has_pair());
        }

        explicit inline Lock(T &&value) : value_{etl::move(value)} {}

        inline etl::optional<LockGuard<T>> lock() {
            auto pair = value_.try_create_pair();
            if (pair.has_value()) {
                return LockGuard<T>{etl::move(pair.value())};
            } else {
                return etl::nullopt;
            }
        }

        inline bool is_locked() const {
            return value_.has_pair();
        }
    };
}; // namespace nb
