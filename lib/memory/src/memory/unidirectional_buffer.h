#pragma once

#include <etl/memory.h>
#include <etl/optional.h>
#include <stddef.h>

namespace memory {
    template <typename T>
    class UnidirectionalBuffer {
        etl::unique_ptr<T[]> buffer_;
        size_t capacity_;
        size_t head_ = 0;
        size_t tail_ = 0;

      public:
        UnidirectionalBuffer() = delete;
        UnidirectionalBuffer(const UnidirectionalBuffer &) = delete;
        UnidirectionalBuffer(UnidirectionalBuffer &&) = default;
        UnidirectionalBuffer &operator=(const UnidirectionalBuffer &) = delete;
        UnidirectionalBuffer &operator=(UnidirectionalBuffer &&) = default;

        UnidirectionalBuffer(size_t capacity)
            : buffer_{etl::unique_ptr<T[]>(new T[capacity])},
              capacity_{capacity} {}

        inline size_t capacity() const {
            return capacity_;
        }

        inline bool is_full() const {
            return head_ == capacity_;
        }

        inline bool is_empty() const {
            return head_ == tail_;
        }

        inline size_t writable_count() const {
            return capacity_ - head_;
        }

        inline size_t readable_count() const {
            return head_ - tail_;
        }

        inline bool push(T data) {
            if (is_full()) {
                return false;
            } else {
                buffer_[head_++] = data;
                return true;
            }
        }

        inline etl::optional<T> shift() {
            if (is_empty()) {
                return etl::nullopt;
            } else {
                return buffer_[tail_++];
            }
        }
    };
} // namespace memory
