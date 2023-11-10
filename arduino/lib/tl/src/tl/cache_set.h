#pragma once

#include "./vec.h"

namespace tl {
    template <uint8_t N>
    class CircularIndex {
        uint8_t index_{0};

      public:
        inline uint8_t offset(uint8_t offset) const {
            return (index_ + offset) % N;
        }

        inline void advance() {
            index_ = (index_ + 1) % N;
        }

        inline bool operator==(const CircularIndex &other) const {
            return index_ == other.index_;
        }

        inline bool operator!=(const CircularIndex &other) const {
            return index_ != other.index_;
        }
    };

    template <uint8_t N>
    class IndexMap {
        etl::array<uint8_t, N> map_{};
        CircularIndex<N> head_{};
        uint8_t size_{0};

      public:
        IndexMap() {
            for (uint8_t i = 0; i < N; i++) {
                map_[i] = i;
            }
        }

        inline uint8_t size() const {
            return size_;
        }

        inline bool full() const {
            return size_ == N;
        }

        inline void pop_oldest() {
            head_.advance();
            size_--;
        }

        inline uint8_t push() {
            if (full()) {
                pop_oldest();
            }

            uint8_t real_end_index = head_.offset(size_);
            size_++;
            return map_[real_end_index];
        }

        inline void remove(uint8_t virt_index) {
            ASSERT(virt_index < size_);
            ASSERT(size_ > 0);

            size_--;
            uint8_t real_index = map_[head_.offset(virt_index)];
            for (uint8_t i = virt_index; i < size_; i++) {
                map_[head_.offset(i)] = map_[head_.offset(i + 1)];
            }
            map_[head_.offset(size_)] = real_index;
        }

        inline uint8_t resolve(uint8_t virt_index) const {
            return map_[head_.offset(virt_index)];
        }
    };

    /**
     * 割と効率よく`T`を格納するキャッシュ．
     * 適当に書いたけどテスト通っちゃった．
     *
     * `T`のデストラクタは呼ばれないので注意．
     */
    template <typename T, uint8_t N>
    class CacheSet {
        etl::array<memory::MaybeUninit<T>, N> entries_{};
        IndexMap<N> index_map_{};

      public:
        inline bool full() const {
            return index_map_.full();
        }

        inline void pop_oldest() {
            index_map_.pop_oldest();
        }

        inline void push(const T &entry) {
            uint8_t real_index = index_map_.push();
            entries_[real_index].set(entry);
        }

        inline void remove(uint8_t index) {
            index_map_.remove(index);
        }

        inline etl::optional<etl::reference_wrapper<const T>> get(uint8_t index) const {
            if (index >= index_map_.size()) {
                return etl::nullopt;
            }
            uint8_t real_index = index_map_.resolve(index);
            return etl::cref(entries_[real_index].get());
        }

        template <typename F>
        inline etl::optional<uint8_t> find_index(F &&f) const {
            for (uint8_t virt_index = 0; virt_index < index_map_.size(); virt_index++) {
                uint8_t real_index = index_map_.resolve(virt_index);
                auto entry = entries_[real_index].get();
                if (f(entry)) {
                    return virt_index;
                }
            }
            return etl::nullopt;
        }

        template <typename F>
        inline bool find_if(F &&f) const {
            return find_index(etl::forward<F>(f)).has_value();
        }
    };
} // namespace tl
