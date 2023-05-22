#pragma once

#include <collection/raw_buffer.hpp>
#include <etl/functional.h>
#include <etl/memory.h>
#include <etl/optional.h>
#include <etl/utility.h>
#include <halt.hpp>

namespace collection {

/**
 * 可変長の要素を格納するコンテナ型．
 * stl::vector<T>に似ているメソッドを持つ．
 *
 * 要素はヒープに確保される．
 *
 * # Example
 *
 * ```
 * #include <collection/vector.hpp>
 *
 * void setup() {
 *     collection::Vector<int> vec;
 *     vec.push_back(42);
 *     vec.push_back(53);
 *
 *     assert(vec[0], 42);
 * }
 * ```
 */
template <typename T>
class Vector {
    RawBuffer<T> buffer_;
    size_t size_;

  public:
    /**
     * 空の`Vector`を作成する．
     *
     * この時点ではまだメモリは確保されていない．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * ```
     *
     */
    Vector() : buffer_(), size_(0) {}

    /**
     * 引数の`capacity`だけメモリを確保した`Vector`を作成する．
     *
     * # Example
     *
     * ```
     * // int型42個分のメモリをあらかじめ確保する．
     * collection::Vector<int> v(42);
     * ```
     */
    Vector(size_t capacity) : buffer_(capacity), size_(0) {}

    Vector(Vector &&other) : buffer_(etl::move(other.buffer_)), size_(other.size_) {
        other.size_ = 0;
    }

    ~Vector<T>() {
        for (size_t i = 0; i < size_; i++) {
            buffer_.destruct_at(i);
        }
    }

    Vector &operator=(Vector &&other) {
        if (this != &other) {
            buffer_ = etl::move(other.buffer_);
            size_ = other.size_;

            other.size_ = 0;
        }
        return *this;
    }

    /**
     * `index`番目の要素にアクセスする．
     *
     * @throw halt `index` >= `size_`である場合
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     * assert(v[0] == 42);
     * ```
     */
    T &operator[](size_t index) {
        if (index >= size_) {
            halt();
        }
        return buffer_.at(index);
    }

    /** @copydoc operator[](size_t) */
    const T &operator[](size_t index) const {
        if (index >= size_) {
            halt();
        }
        return buffer_.at(index);
    }

    /**
     * メモリを再確保せずに格納できる，最大の要素数を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v(42);
     * assert(v.capacity() == 42);
     * ```
     */
    size_t capacity() const {
        return buffer_.capacity();
    }

    /**
     * 要素数を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     * assert(v.size() == 1);
     * ```
     *
     */
    size_t size() const {
        return size_;
    }

    /**
     * 空であるかを返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * assert(v.is_empty());
     * ```
     */
    bool is_empty() const {
        return size_ == 0;
    }

    /**
     * 確保したメモリ領域を`size_`まで切り詰める
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v(100);
     * assert(v.capacity() == 100);
     *
     * v.shrink_to_fit();
     * assert(v.capacity() == 0);
     * ```
     */
    void shrink_to_fit() {
        if (size_ < buffer_.capacity()) {
            auto new_buffer = RawBuffer<T>(size_);
            new_buffer.move_from(0, 0, size_, buffer_);
            buffer_ = etl::move(new_buffer);
        }
    }

    /**
     * 末尾に要素を追加する．
     *
     * # Example
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     *
     * assert(v[0] == 42);
     *
     * ```
     */
    void push_back(T &&value) {
        const size_t capacity = buffer_.capacity();
        if (size_ >= capacity) {
            auto new_buffer = RawBuffer<T>(capacity == 0 ? 1 : capacity * 2);
            new_buffer.move_from(0, 0, size_, buffer_);
            buffer_ = etl::move(new_buffer);
        }

        buffer_.set(size_, etl::move(value));
        size_++;
    }

    /** @copydoc Vector::push_back(T &&) */
    void push_back(T &value) {
        T copied = value;
        push_back(etl::move(copied));
    }

    /**
     * `index`番目の位置に要素を挿入する．
     *
     * @throw halt `index` > `size_`である場合
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back()
     * ```
     */
    void insert(size_t index, T &&value) {
        if (index > size_) {
            halt();
        }

        const size_t capacity = buffer_.capacity();

        // メモリが足りている場合
        if (size_ < capacity) {
            buffer_.move_from(index + 1, index, size_ - index, buffer_);
            buffer_.set(index, etl::move(value));
            size_++;
            return;
        }

        // メモリが不足している場合，新しいバッファに要素を移す．
        auto new_buffer = RawBuffer<T>(capacity == 0 ? 1 : capacity * 2);
        new_buffer.move_from(0, 0, index, buffer_);
        new_buffer.set(index, etl::move(value));
        new_buffer.move_from(index + 1, index, size_ - index, buffer_);
        buffer_ = etl::move(new_buffer);
        size_++;
    }

    /** @copydoc Vector::insert(size_t, T &&) */
    void insert(size_t index, T &value) {
        T copied = value;
        insert(index, etl::move(copied));
    }

    /**
     * 末尾の要素を削除する
     *
     * 範囲外の場合は空の値を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     * v.push_back(53);
     * assert(v.pop_back() == etl::optional(53));
     * assert(v.pop_back() == etl::optional(42));
     * assert(v.pop_back() == etl::nullopt);
     * ```
     */
    etl::optional<T> pop_back() {
        if (size_ == 0) {
            return etl::nullopt;
        }

        T erased = buffer_.move_at(size_ - 1);
        size_--;

        return erased;
    }

    /**
     * `index`番目の要素を削除し，空いたスペース続く要素で詰める．
     *
     * 削除した要素を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     * assert(v.erase(0) == 42);
     * ```
     */
    T erase(size_t index) {
        if (index >= size_) {
            halt();
        }

        T erased = buffer_.move_at(index);
        buffer_.move_from(index, index + 1, size_ - index - 1, buffer_);
        size_--;

        return erased;
    }

    /**
     * `index`番目の要素にアクセスする．
     *
     * 範囲外の場合は空の値を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * v.push_back(42);
     * assert(v.at(0), 42);
     * assert(v.at(1), etl::nullopt);
     *```
     */
    etl::optional<etl::reference_wrapper<T>> at(size_t index) {
        if (index >= size_) {
            return etl::nullopt;
        }
        return etl::ref(buffer_.at(index));
    }

    /** @copydoc Vector::at(size_t) */
    etl::optional<etl::reference_wrapper<const T>> at(size_t index) const {
        if (index >= size_) {
            return etl::nullopt;
        }
        return etl::ref(buffer_.at(index));
    }

    /**
     * 先頭の要素にアクセスする．
     *
     * 存在しない場合は空の値を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * assert(v.front(), etl::nullopt);
     *
     * v.push_back(42);
     * v.push_back(53);
     * assert(v.front(), 42);
     *```
     */
    etl::optional<etl::reference_wrapper<T>> front() {
        return at(0);
    }

    /** @copydoc Vector::front() */
    etl::optional<etl::reference_wrapper<const T>> front() const {
        return at(0);
    }

    /**
     * 末尾の要素にアクセスする．
     *
     * 存在しない場合は空の値を返す．
     *
     * # Example
     *
     * ```
     * collection::Vector<int> v;
     * assert(v.back(), etl::nullopt);
     *
     * v.push_back(42);
     * v.push_back(53);
     * assert(v.back(), 42);
     *```
     */
    etl::optional<etl::reference_wrapper<T>> back() {
        if (size_ == 0) {
            return etl::nullopt;
        }
        return at(size_ - 1);
    }

    /** @copydoc Vector::back() */
    etl::optional<etl::reference_wrapper<const T>> back() const {
        if (size_ == 0) {
            return etl::nullopt;
        }
        return at(size_ - 1);
    }

    T *begin() {
        return buffer_.data();
    }

    const T *begin() const {
        return buffer_.data();
    }

    T *end() {
        return buffer_.data() + size_;
    }

    const T *end() const {
        return buffer_.data() + size_;
    }

    const T *cbegin() const {
        return buffer_.data();
    }

    const T *cend() const {
        return buffer_.data() + size_;
    }
};

} // namespace collection
