#pragma once

#include "shared/inner.h"

namespace memory {
    template <typename T>
    class Shared {
        Inner<T> *inner_;

        inline void dettach_inner() {
            if (inner_) {
                inner_->decrement_count();
                if (inner_->is_empty()) {
                    delete inner_;
                }
            }
        }

      public:
        Shared() : inner_{new Inner<T>{}} {};

        Shared(Shared &&other) = delete;
        Shared &operator=(Shared &&other) = delete;

        Shared(const Shared &other) {
            inner_ = other.inner_;
            inner_->increment_count();
        }

        Shared &operator=(const Shared &other) {
            dettach_inner();
            inner_ = other.inner_;
            inner_->increment_count();
            return *this;
        }

        ~Shared() {
            dettach_inner();
        }

        Shared(T &&value) {
            inner_ = new Inner<T>{etl::move(value)};
        }

        T *operator->() {
            return &inner_->operator*();
        }

        const T *operator->() const {
            return &inner_->operator*();
        }

        T &operator*() {
            return inner_->operator*();
        }

        const T &operator*() const {
            return inner_->operator*();
        }

        uint8_t ref_count() const {
            return inner_->ref_count();
        }

        bool is_unique() const {
            return inner_->is_unique();
        }
    };
} // namespace memory
