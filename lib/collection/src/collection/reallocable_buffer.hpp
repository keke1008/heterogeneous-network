#pragma once

#include <etl/memory.h>
#include <halt.hpp>
#include <stdlib.h>

namespace collection {

template <typename T>
class ReallocableBuffer {
    static_assert(etl::is_trivially_copyable_v<T>);

    size_t capacity_;
    T *data_;

    constexpr void assert_is_valid_index(size_t index) const {
        if (index >= capacity_) {
            halt();
        }
    }

  public:
    ReallocableBuffer() : data_(nullptr), capacity_(0) {}

    ReallocableBuffer(size_t capacity) : capacity_(capacity) {
        data_ = static_cast<T *>(calloc(capacity, sizeof(T)));
    }

    ReallocableBuffer(const ReallocableBuffer &) = delete;
    ReallocableBuffer &operator=(const ReallocableBuffer &) = delete;

    ReallocableBuffer(ReallocableBuffer &&other) : capacity_(other.capacity_) {
        delete data_;
        data_ = other.data_;
    }
};

} // namespace collection
