#pragma once

#include "./vec.h"
#include <stdint.h>

namespace tl {
    template <typename T, uint8_t N>
    class Set {
        Vec<T, N> vec_;

      public:
        Set() = default;
        Set(const Set &other) = default;
        Set(Set &&other) = default;
        Set &operator=(const Set &other) = default;
        Set &operator=(Set &&other) = default;

        explicit inline Set(const Vec<T, N> &vec) : vec_{vec} {}

        explicit inline Set(Vec<T, N> &&vec) : vec_{vec} {}

        explicit inline Set(const T *begin, const T *end) : vec_{begin, end} {}

        explicit inline Set(const etl::span<const T> &span) : vec_{span} {}

        explicit inline Set(const std::initializer_list<T> &list) : vec_{list} {}

        inline bool operator==(const Set &other) const {
            if (vec_.size() != other.vec_.size()) {
                return false;
            }
            for (uint8_t i = 0; i < vec_.size(); i++) {
                if (!other.contains(vec_[i])) {
                    return false;
                }
            }
            return true;
        }

        inline bool operator!=(const Set &other) const {
            return !(*this == other);
        }

        inline uint8_t size() const {
            return vec_.size();
        }

        inline uint8_t capacity() const {
            return vec_.capacity();
        }

        inline bool empty() const {
            return vec_.empty();
        }

        inline bool full() const {
            return vec_.full();
        }

        inline T &front() {
            return vec_.front();
        }

        inline const T &front() const {
            return vec_.front();
        }

        inline T &back() {
            return vec_.back();
        }

        inline const T &back() const {
            return vec_.back();
        }

        inline T *begin() {
            return vec_.begin();
        }

        inline const T *begin() const {
            return vec_.begin();
        }

        inline T *end() {
            return vec_.end();
        }

        inline const T *end() const {
            return vec_.end();
        }

        inline etl::span<T> as_span() {
            return vec_.as_span();
        }

        inline etl::span<const T> as_span() const {
            return vec_.as_span();
        }

        inline bool contains(const T &value) const {
            for (uint8_t i = 0; i < vec_.size(); i++) {
                if (vec_[i] == value) {
                    return true;
                }
            }
            return false;
        }

        inline uint8_t count(const T &value) const {
            uint8_t count = 0;
            for (uint8_t i = 0; i < vec_.size(); i++) {
                if (vec_[i] == value) {
                    count++;
                }
            }
            return count;
        }

        inline void insert_uncheked(const T &value) {
            vec_.push_back(value);
        }

        inline void insert_uncheked(T &&value) {
            vec_.push_back(etl::move(value));
        }

        inline void insert(const T &value) {
            if (contains(value)) {
                return;
            }
            insert_uncheked(value);
        }

        inline void insert(T &&value) {
            if (contains(value)) {
                return;
            }
            insert_uncheked(etl::move(value));
        }

        inline void remove(const T &value) {
            for (uint8_t i = 0; i < vec_.size(); i++) {
                if (vec_[i] == value) {
                    vec_.swap_remove(i);
                    return;
                }
            }
        }

        inline T pop_front() {
            return vec_.remove(0);
        }

        inline T pop_back() {
            return vec_.pop_back();
        }

        inline void clear() {
            vec_.clear();
        }
    };
} // namespace tl
