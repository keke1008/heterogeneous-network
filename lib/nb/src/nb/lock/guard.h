#pragma once

#include "mutex.h"

#include <etl/optional.h>

namespace nb::lock {
    template <typename T>
    class Guard {
        Mutex *mutex_;
        T *value_;

      public:
        Guard() = delete;
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

        Guard(Guard &&other) : mutex_{other.mutex_}, value_{other.value_} {
            other.mutex_ = nullptr;
            other.value_ = nullptr;
        }

        Guard &operator=(Guard &&other) {
            if (this != &other) {
                if (mutex_ != other.mutex_) {
                    mutex_->unlock();
                    mutex_ = other.mutex_;
                }
                other.mutex_ = nullptr;

                value_ = other.value_;
                other.value_ = nullptr;
            }
            return *this;
        }

        explicit Guard(Mutex *mutex, T *value) : mutex_{mutex}, value_{value} {}

        ~Guard() {
            if (mutex_ != nullptr) {
                mutex_->unlock();
            }
        }

        [[deprecated("use get() instead")]] T &operator*() & {
            return *value_;
        }

        [[deprecated("use get() instead")]] const T &operator*() const & {
            return *value_;
        }

        [[deprecated("use get() instead")]] T *operator->() {
            return value_;
        }

        [[deprecated("use get() instead")]] const T *operator->() const {
            return value_;
        }

        T &get() & {
            return *value_;
        }

        const T &get() const & {
            return *value_;
        }
    };
} // namespace nb::lock
