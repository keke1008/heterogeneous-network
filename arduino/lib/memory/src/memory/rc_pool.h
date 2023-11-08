#pragma once

#include <etl/list.h>
#include <etl/optional.h>
#include <etl/utility.h>
#include <memory/lifetime.h>
#include <stdint.h>

namespace memory {
    class RcPoolCounter {
        uint8_t count_ = 0;

      public:
        RcPoolCounter() = default;
        RcPoolCounter(const RcPoolCounter &) = delete;
        RcPoolCounter(RcPoolCounter &&) = default;
        RcPoolCounter &operator=(const RcPoolCounter &) = delete;
        RcPoolCounter &operator=(RcPoolCounter &&) = default;

        RcPoolCounter(uint8_t count) : count_{count} {}

        uint8_t count() const {
            return count_;
        }

        void increment() {
            ++count_;
        }

        void decrement() {
            --count_;
        }

        bool is_zero() const {
            return count_ == 0;
        }
    };

    template <typename T>
    union DeferredInit {
      private:
        uint8_t uninitialized[0];
        T value;

      public:
        DeferredInit() : uninitialized{} {}

        T *as_ptr() {
            return &value;
        }

        ~DeferredInit() {
            value.~T();
        }
    };

    template <typename T>
    class RcPoolEntry {
        RcPoolCounter counter_;
        DeferredInit<T> value_;

      public:
        RcPoolEntry() = default;
        RcPoolEntry(const RcPoolEntry &) = delete;
        RcPoolEntry(RcPoolEntry &&) = delete;
        RcPoolEntry &operator=(const RcPoolEntry &) = delete;
        RcPoolEntry &operator=(RcPoolEntry &&) = delete;

        T *value() {
            return value_.as_ptr();
        }

        RcPoolCounter *counter() {
            return &counter_;
        }
    };

    template <typename T>
    class RcPoolRef;

    template <typename T, uint8_t N>
    class RcPool {
        friend class RcPoolRef<T>;
        etl::list<RcPoolEntry<T>, N> entries_;

      public:
        RcPool() = default;
        RcPool(const RcPool &) = delete;
        RcPool(RcPool &&) = delete;
        RcPool &operator=(const RcPool &) = delete;
        RcPool &operator=(RcPool &&) = delete;
    };

    template <typename T>
    class RcPoolRef {
        StaticRef<etl::ilist<RcPoolEntry<T>>> entries_;

      public:
        RcPoolRef() = delete;
        RcPoolRef(const RcPoolRef &) = default;
        RcPoolRef(RcPoolRef &&) = default;
        RcPoolRef &operator=(const RcPoolRef &) = default;
        RcPoolRef &operator=(RcPoolRef &&) = default;

        template <uint8_t N>
        RcPoolRef(Static<RcPool<T, N>> &pool) : entries_{pool.get().entries_} {}

      private:
        inline void remove_zero_count_entries() {
            entries_->remove_if([&](RcPoolEntry<T> &entry) { return entry.counter()->is_zero(); });
        }

      public:
        etl::optional<etl::pair<RcPoolCounter *, T *>> allocate() {
            if (entries_->full()) {
                remove_zero_count_entries();
                if (entries_->full()) {
                    return etl::nullopt;
                }
            }

            auto &entry = entries_->emplace_back();
            return etl::make_pair(entry.counter(), entry.value());
        }
    };
} // namespace memory
