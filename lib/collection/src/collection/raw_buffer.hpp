#pragma once

#include <etl/functional.h>
#include <etl/memory.h>
#include <etl/optional.h>
#include <etl/utility.h>
#include <halt.hpp>

namespace collection {

/**
 * `T`型の固定長配列．
 *
 * 要素に対するムーブができるため，有効なメモリが連続しない状態を作れる．
 * アクセスには細心の注意を払うこと．
 *
 * @warning このクラスは，デストラクタが呼ばれた際に格納している要素のデストラクタを呼び出さない．
 * 必要ならば，手動で`RawBuffer::destruct_at`を呼び出すこと．
 */
template <typename T>
class RawBuffer {
    size_t capacity_;
    T *data_;

    constexpr void assert_valid_index(size_t index) const {
        if (index >= capacity_) {
            halt();
        }
    }

  public:
    /**
     * 空の`RawBuffer`を作成する．
     *
     * メモリのアロケーションは行わない．
     */
    RawBuffer() : data_(nullptr), capacity_(0) {}

    /**
     * `capacity`だけメモリを確保した`RawBuffer`を作成する．
     */
    RawBuffer(size_t capacity) : capacity_(capacity) {
        data_ = static_cast<T *>(operator new(capacity * sizeof(T)));
        if (!data_) {
            halt();
        }
    }

    RawBuffer(const RawBuffer &) = delete;
    RawBuffer &operator=(const RawBuffer &) = delete;

    RawBuffer(RawBuffer &&other) : capacity_(other.capacity_) {
        delete data_;
        data_ = other.data_;

        other.data_ = nullptr;
        other.capacity_ = 0;
    }

    RawBuffer &operator=(RawBuffer &&other) {
        if (this != &other) {
            delete data_;
            data_ = other.data_;
            capacity_ = other.capacity_;

            other.data_ = nullptr;
            other.capacity_ = 0;
        }
        return *this;
    }

    /**
     * @attention 要素ごとのデストラクタは呼び出さない
     */
    ~RawBuffer() {
        delete data_;
    }

    /**
     * 格納可能な最大容量を取得する．
     */
    size_t capacity() const {
        return capacity_;
    }

    /**
     * 要素が格納されている配列の先頭ポインタを取得する．
     *
     * アロケーションがされていない場合は`nullptr`を返す．
     */
    T *data() {
        return data_;
    }

    /** @copydoc RawBuffer::data() */
    const T *data() const {
        return data_;
    }

    /**
     * `index`番目の要素にアクセスする．
     *
     * @throw halt `index >= capacity_`の場合
     */
    T &at(size_t index) {
        assert_valid_index(index);
        return data_[index];
    }

    /** @copydoc RawBuffer::at(size_t) */
    const T &at(size_t index) const {
        assert_valid_index(index);
        return data_[index];
    }

    /**
     * `index`番目の要素の値を設定する．
     *
     * @throw halt `index >= capacity_`の場合
     */
    void set(size_t index, T &&value) {
        assert_valid_index(index);
        data_[index] = etl::move(value);
    }

    /**
     * `index`番目の要素をムーブして返す．
     *
     * @throw halt `index >= capacity_`の場合
     *
     * @warning 型`T`のムーブ後の値が規定されていない場合，ムーブ後の要素にアクセスしてはいけない．
     *
     */
    T move_at(size_t index) {
        assert_valid_index(index);
        auto moved = etl::move(data_[index]);
        return moved;
    }

    /**
     * `other`から`count`個数の要素を自身にムーブさせる．
     *
     * `this`と`other`が同じオブジェクトでもよい
     *
     * @param this_begin ムーブ先の開始インデックス
     * @param other_begin ムーブ元の開始インデックス
     * @param count ムーブする要素の個数
     * @param other ムーブ元の`RawBuffer`
     *
     * @throw 移動元移動先ともにcapacity_が十分で無い場合
     *
     * @warning 実行後`other`は一部の要素のみムーブされた状態となる．
     */
    void move_from(size_t this_begin, size_t other_begin, size_t count, RawBuffer<T> &other) {
        if (this_begin + count > capacity_ || other_begin + count > other.capacity_) {
            halt();
        }

        // 前の要素からムーブしてよい，もしくはする必要がある場合
        if (this != &other || this_begin < other_begin) {
            for (size_t i = 0; i < count; i++) {
                data_[this_begin + i] = etl::move(other.data_[other_begin + i]);
            }
            return;
        }

        // 後ろの要素からムーブする必要がある場合
        if (this_begin > other_begin) {
            for (size_t i = 0; i < count; i++) {
                const size_t offset = count - i - 1;
                data_[this_begin + offset] = etl::move(other.data_[other_begin + offset]);
            }
        }
    }

    /**
     * `index`番目の要素のデストラクタを呼び出す．
     *
     * @warning 呼ぶ出したインデックスの要素は，新しい値を入れるまでアクセスすべきでない．
     *
     * @throw halt `index >= capacity_`の場合
     */
    void destruct_at(size_t index) {
        assert_valid_index(index);
        data_[index].~T();
    }
};

} // namespace collection
