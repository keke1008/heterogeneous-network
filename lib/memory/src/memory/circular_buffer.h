#pragma once

#include "./buffer.h"
#include <etl/memory.h>
#include <etl/optional.h>
#include <stddef.h>

namespace memory {
    template <
        typename B,
        typename E = etl::decay_t<decltype(etl::declval<B>()[etl::declval<size_t>()])>>
    class CircularBuffer {
        static_assert(is_buffer_v<B, E>, "B must be a buffer of E");

        size_t capacity_;
        size_t size_ = 0;
        size_t head_ = 0;
        size_t tail_ = 0;
        B buffer_;

        void advance_head() {
            head_ = (head_ + 1) % capacity_;
            size_++;
        }

        void advance_tail() {
            tail_ = (tail_ + 1) % capacity_;
            size_--;
        }

        void retreat_head() {
            head_ = (head_ - 1) % capacity_;
            size_--;
        }

        void retreat_tail() {
            tail_ = (tail_ - 1) % capacity_;
            size_++;
        }

      public:
        CircularBuffer() = delete;
        CircularBuffer(const CircularBuffer &) = default;
        CircularBuffer &operator=(const CircularBuffer &) = default;
        CircularBuffer(CircularBuffer &&other) = default;
        CircularBuffer &operator=(CircularBuffer &&other) = default;

        CircularBuffer(B &&buffer, size_t capacity)
            : buffer_{etl::forward<B>(buffer)},
              capacity_{capacity} {}

        CircularBuffer(size_t capacity) : buffer_{}, capacity_{capacity} {}

        inline size_t capacity() const {
            return capacity_;
        }

        inline bool is_empty() const {
            return size_ == 0;
        }

        inline bool is_full() const {
            return size_ == capacity_;
        }

        inline size_t vacant_count() const {
            return capacity_ - size_;
        }

        inline size_t occupied_count() const {
            return size_;
        }

        void clear() {
            size_ = 0;
            head_ = 0;
            tail_ = 0;
        }

        inline etl::optional<E> peek_back() const {
            if (is_empty()) {
                return etl::nullopt;
            }

            return buffer_[(head_ - 1) % capacity_];
        }

        inline etl::optional<E> peek_front() const {
            if (is_empty()) {
                return etl::nullopt;
            }

            return buffer_[tail_];
        }

        inline void force_push_back(E item) {
            buffer_[head_] = etl::move(item);
            advance_head();
        }

        inline bool push_back(E item) {
            if (is_full()) {
                return false;
            }
            force_push_back(item);
            return true;
        }

        inline void force_push_front(E item) {
            retreat_tail();
            buffer_[tail_] = etl::move(item);
        }

        inline bool push_front(E item) {
            if (is_full()) {
                return false;
            }
            force_push_front(item);
            return true;
        }

        inline etl::optional<E> pop_back() {
            if (is_empty()) {
                return etl::nullopt;
            }

            retreat_head();
            return buffer_[head_];
        }

        inline etl::optional<E> pop_front() {
            if (is_empty()) {
                return etl::nullopt;
            }

            auto item = buffer_[tail_];
            advance_tail();
            return item;
        }

        static CircularBuffer<etl::unique_ptr<E[]>, E> make_heap(size_t capacity) {
            return CircularBuffer<etl::unique_ptr<E[]>, E>{
                etl::unique_ptr<E[]>(new E[capacity]),
                capacity,
            };
        }

        template <size_t N>
        static CircularBuffer<E[N], E> make_stack() {
            return CircularBuffer<E[N], E>{N};
        }
    };
} // namespace memory
