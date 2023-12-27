#pragma once

#include <etl/array.h>
#include <etl/initializer_list.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <logger.h>
#include <memory/maybe_uninit.h>

namespace tl {
    template <typename T, uint8_t N>
    class Vec {
        etl::array<memory::MaybeUninit<T>, N> data_;
        uint8_t size_;

        void destroy() {
            for (uint8_t i = 0; i < size_; i++) {
                data_[i].destroy();
            }
        }

        void copy_from(const Vec<T, N> &other) {
            FASSERT(this != &other);
            for (uint8_t i = 0; i < other.size_; i++) {
                data_[i].set(other.data_[i].get());
            }
            size_ = other.size_;
        }

        void move_from(Vec<T, N> &&other) {
            FASSERT(this != &other);
            for (uint8_t i = 0; i < other.size_; i++) {
                data_[i].set(etl::move(other.data_[i].get()));
            }
            size_ = other.size_;
            other.size_ = 0;
        }

      public:
        inline Vec() : size_{0} {};

        inline Vec(const Vec<T, N> &other) {
            copy_from(other);
        }

        inline Vec(Vec<T, N> &&other) {
            move_from(etl::move(other));
        }

        inline Vec<T, N> &operator=(const Vec<T, N> &other) {
            if (this == &other) {
                return *this;
            }

            destroy();
            copy_from(other);
            return *this;
        }

        inline Vec<T, N> &operator=(Vec<T, N> &&other) {
            if (this == &other) {
                return *this;
            }

            destroy();
            move_from(etl::move(other));
            return *this;
        }

        inline Vec(const T *begin, const T *end) : size_{0} {
            assign(begin, end);
        }

        inline Vec(const etl::span<const T> &span) : size_{0} {
            assign(span.begin(), span.end());
        }

        inline Vec(std::initializer_list<T> list) : size_{0} {
            assign(list.begin(), list.end());
        }

        inline ~Vec() {
            destroy();
        }

        bool operator==(const Vec<T, N> &other) const {
            if (size_ != other.size_) {
                return false;
            }
            for (uint8_t i = 0; i < size_; i++) {
                if (data_[i].get() != other.data_[i].get()) {
                    return false;
                }
            }
            return true;
        }

        inline bool operator!=(const Vec<T, N> &other) const {
            return !(*this == other);
        }

        inline uint8_t size() const {
            return size_;
        }

        inline uint8_t capacity() const {
            return N;
        }

        inline bool empty() const {
            return size_ == 0;
        }

        inline bool full() const {
            return size_ == N;
        }

        inline T &operator[](uint8_t index) {
            return data_[index].get();
        }

        inline const T &operator[](uint8_t index) const {
            return data_[index].get();
        }

        inline etl::optional<etl::reference_wrapper<T>> at(uint8_t index) {
            if (index < size_) {
                return etl::ref(data_[index].get());
            } else {
                return etl::nullopt;
            }
        }

        inline etl::optional<etl::reference_wrapper<const T>> at(uint8_t index) const {
            if (index < size_) {
                return etl::cref(data_[index].get());
            } else {
                return etl::nullopt;
            }
        }

        inline T &front() {
            return data_.front().get();
        }

        inline const T &front() const {
            return data_.front().get();
        }

        inline T &back() {
            return data_[size_ - 1].get();
        }

        inline const T &back() const {
            return data_[size_ - 1].get();
        }

        inline T *begin() {
            return &data_.front().get();
        }

        inline const T *begin() const {
            return &data_.front().get();
        }

        inline T *end() {
            return &data_[size_].get();
        }

        inline const T *end() const {
            return &data_[size_].get();
        }

        inline etl::span<T> as_span() {
            return etl::span<T>(begin(), size_);
        }

        inline etl::span<const T> as_span() const {
            return etl::span<const T>(begin(), size_);
        }

        void assign(const T *begin, const T *end) {
            ASSERT(etl::distance(begin, end) <= N);

            uint8_t distance = etl::distance(begin, end);
            uint8_t overlap_size = etl::min(size_, distance);
            for (uint8_t i = 0; i < overlap_size; i++) {
                data_[i].destroy_and_set(begin[i]);
            }
            for (uint8_t i = overlap_size; i < distance; i++) {
                data_[i].set(begin[i]);
            }
        }

        inline void push_back(const T &value) {
            ASSERT(size_ < N);
            data_[size_].set(value);
            size_++;
        }

        inline void push_back(T &&value) {
            ASSERT(size_ < N);
            data_[size_].set(etl::move(value));
            size_++;
        }

        template <typename... Args>
        inline void emplace_back(Args &&...args) {
            ASSERT(size_ < N);
            data_[size_].set(T(etl::forward<Args>(args)...));
            size_++;
        }

        inline T pop_back() {
            ASSERT(size_ > 0);
            size_--;

            memory::MaybeUninit<T> &ref = data_[size_];
            T value = etl::move(ref.get());
            ref.destroy();
            return value;
        }

        inline void clear() {
            destroy();
            size_ = 0;
        }

        void insert(uint8_t index, const T &value) {
            ASSERT(index <= size_);
            ASSERT(size_ < N);

            for (uint8_t i = size_; i > index; i--) {
                memory::MaybeUninit<T> &from = data_[i - 1];
                data_[i].set(etl::move(from.get()));
                from.destroy();
            }
            data_[index].set(value);
            size_++;
        }

        void insert(uint8_t index, T &&value) {
            ASSERT(index <= size_);
            ASSERT(size_ < N);

            for (uint8_t i = size_; i > index; i--) {
                memory::MaybeUninit<T> &from = data_[i - 1];
                data_[i].set(etl::move(from.get()));
                from.destroy();
            }
            data_[index].set(etl::move(value));
            size_++;
        }

        T remove(uint8_t index) {
            ASSERT(index < size_);

            T value = etl::move(data_[index].get());
            for (uint8_t i = index; i < size_ - 1; i++) {
                memory::MaybeUninit<T> &from = data_[i + 1];
                data_[i].set(etl::move(from.get()));
                from.destroy();
            }
            size_--;
            return value;
        }

        T swap_remove(uint8_t index) {
            ASSERT(index < size_);

            T value = etl::move(data_[index].get());
            data_[index].destroy();
            size_--;

            if (index != size_) {
                data_[index].set(etl::move(data_[size_].get()));
            }

            return value;
        }
    };
} // namespace tl
