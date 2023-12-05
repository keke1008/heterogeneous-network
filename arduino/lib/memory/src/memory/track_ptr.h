#pragma once

#include <etl/functional.h>
#include <etl/optional.h>
#include <etl/utility.h>

namespace memory {
    template <typename T, typename U>
    class TrackPtr {
        friend class TrackPtr<U, T>;
        TrackPtr<U, T> *ptr_;
        T value_;

        TrackPtr(T &&value) : ptr_{nullptr}, value_{etl::move(value)} {}

      public:
        TrackPtr() = delete;
        TrackPtr(const TrackPtr &) = delete;
        TrackPtr &operator=(const TrackPtr &) = delete;

        TrackPtr(TrackPtr &&other) noexcept : ptr_{other.ptr_} {
            value_ = etl::move(other.value_);
            if (ptr_ != nullptr) {
                ptr_->ptr_ = this;
            }
            other.ptr_ = nullptr;
        }

        TrackPtr &operator=(TrackPtr &&other) noexcept {
            if (this != &other) {
                value_ = etl::move(other.value_);
                if (ptr_ != nullptr) {
                    ptr_->ptr_ = nullptr;
                }
                ptr_ = other.ptr_;
                if (other.ptr_ != nullptr) {
                    other.ptr_->ptr_ = this;
                    other.ptr_ = nullptr;
                }
            }
            return *this;
        }

        ~TrackPtr() {
            if (ptr_ != nullptr) {
                ptr_->ptr_ = nullptr;
            }
        }

        static etl::pair<TrackPtr<T, U>, TrackPtr<U, T>> make_track_pair(T &&t, U &&u) {
            auto t_ptr = TrackPtr<T, U>(etl::forward<T>(t));
            auto u_ptr = TrackPtr<U, T>(etl::forward<U>(u));
            t_ptr.ptr_ = &u_ptr;
            u_ptr.ptr_ = &t_ptr;
            return etl::make_pair(etl::move(t_ptr), etl::move(u_ptr));
        }

        inline T &get() {
            return value_;
        }

        inline etl::optional<etl::reference_wrapper<U>> try_get_pair() {
            if (ptr_ == nullptr) {
                return etl::nullopt;
            }
            return etl::ref(ptr_->value_);
        }
    };

    template <typename T, typename U>
    etl::pair<TrackPtr<T, U>, TrackPtr<U, T>> make_track_ptr(T &&t, U &&u) {
        return TrackPtr<T, U>::make_track_pair(etl::forward<T>(t), etl::forward<U>(u));
    }
} // namespace memory
