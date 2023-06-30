#pragma once

#include "guard.h"
#include "mutex.h"
#include <etl/optional.h>
#include <etl/utility.h>

namespace nb {
    template <typename T>
    class Lock {
        lock::Mutex mutex_;
        T value_;

      public:
        explicit Lock(const T &value) = delete;

        explicit Lock(T &&value) : value_{etl::move(value)} {}

        etl::optional<lock::Guard<T>> lock() & {
            if (mutex_.lock()) {
                return lock::Guard(&mutex_, &value_);
            } else {
                return etl::nullopt;
            }
        }
    };
} // namespace nb
