#pragma once

#include "./poll.h"

namespace nb {
    namespace lock {
        class MutexGuard {
            bool *locked_;

          public:
            MutexGuard() = delete;
            MutexGuard(const MutexGuard &) = delete;

            MutexGuard(MutexGuard &&other) : locked_{other.locked_} {
                other.locked_ = nullptr;
            }

            MutexGuard &operator=(const MutexGuard &) = delete;

            MutexGuard &operator=(MutexGuard &&other) {
                locked_ = other.locked_;
                other.locked_ = nullptr;
                return *this;
            }

            inline MutexGuard(bool &locked) : locked_{&locked} {
                FASSERT(!locked);
                *locked_ = true;
            }

            inline ~MutexGuard() {
                if (locked_ != nullptr) {
                    FASSERT(*locked_);
                    *locked_ = false;
                    locked_ = nullptr;
                }
            }

            inline bool has_lock() const {
                return locked_ != nullptr && *locked_;
            }
        };

        class Mutex {
            bool locked_ = false;

          public:
            Mutex() = default;
            Mutex(const Mutex &) = delete;
            Mutex(Mutex &&) = delete;
            Mutex &operator=(const Mutex &) = delete;
            Mutex &operator=(Mutex &&) = delete;

            inline nb::Poll<MutexGuard> poll_lock() {
                return locked_ ? nb::pending : nb::ready(MutexGuard{locked_});
            }

            inline bool is_locked() const {
                return locked_;
            }
        };
    } // namespace lock

    template <typename T>
    class LockGuard {
        T *value_;
        lock::MutexGuard mutex_;

      public:
        LockGuard() = delete;
        LockGuard(const LockGuard &) = delete;
        LockGuard(LockGuard &&) = default;
        LockGuard &operator=(const LockGuard &) = delete;
        LockGuard &operator=(LockGuard &&) = default;

        LockGuard(T &value, lock::MutexGuard &&mutex) : value_{&value}, mutex_{etl::move(mutex)} {}

        inline T &get() {
            FASSERT(mutex_.has_lock());
            return *value_;
        }

        inline T &operator*() {
            FASSERT(mutex_.has_lock());
            return *value_;
        }

        inline T *operator->() {
            FASSERT(mutex_.has_lock());
            return value_;
        }
    };

    template <typename T>
    class Lock {
        T value_;
        lock::Mutex mutex_;

      public:
        Lock() = default;
        Lock(const Lock &) = delete;
        Lock(Lock &&) = delete;
        Lock &operator=(const Lock &) = delete;
        Lock &operator=(Lock &&) = delete;

        explicit inline Lock(const T &value) : value_{value} {}

        explicit inline Lock(T &&value) : value_{etl::move(value)} {}

        inline nb::Poll<LockGuard<T>> poll_lock() {
            auto mutex_guard = POLL_MOVE_UNWRAP_OR_RETURN(mutex_.poll_lock());
            return LockGuard<T>{value_, etl::move(mutex_guard)};
        }

        inline bool is_locked() const {
            return mutex_.is_locked();
        }
    };
} // namespace nb
