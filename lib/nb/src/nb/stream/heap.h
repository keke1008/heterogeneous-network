#pragma once

#include "stream.h"
#include <etl/utility.h>
#include <etl/vector.h>
#include <memory/shared.h>
#include <memory/unidirectional_buffer.h>
#include <stddef.h>

namespace nb::stream {
    template <typename T>
    class HeapStreamWriter {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer_;

      public:
        using WritableStreamItem = T;

        HeapStreamWriter(const HeapStreamWriter &) = delete;
        HeapStreamWriter &operator=(const HeapStreamWriter &) = delete;

        HeapStreamWriter(HeapStreamWriter &&other) : buffer_{other.buffer_} {}

        HeapStreamWriter &operator=(HeapStreamWriter &&other) {
            buffer_ = other.buffer_;
            return *this;
        }

        HeapStreamWriter(memory::Shared<memory::UnidirectionalBuffer<T>> buffer)
            : buffer_{buffer} {}

        size_t writable_count() const {
            return buffer_->writable_count();
        }

        bool write(const T item) {
            return buffer_->push(item);
        }

        bool is_closed() const {
            return buffer_.is_unique();
        }
    };

    static_assert(is_stream_writer_v<HeapStreamWriter<uint8_t>, uint8_t>);

    template <typename T>
    class HeapStreamReader {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer_;

      public:
        using ReadableStreamItem = T;

        HeapStreamReader(const HeapStreamReader &) = delete;
        HeapStreamReader &operator=(const HeapStreamReader &) = delete;

        HeapStreamReader(HeapStreamReader &&other) : buffer_{other.buffer_} {}

        HeapStreamReader &operator=(HeapStreamReader &&other) {
            buffer_ = other.buffer_;
            return *this;
        }

        HeapStreamReader(memory::Shared<memory::UnidirectionalBuffer<T>> buffer)
            : buffer_{buffer} {}

        size_t readable_count() const {
            return buffer_->readable_count();
        }

        etl::optional<T> read() {
            return buffer_->shift();
        }

        bool is_closed() const {
            return buffer_.is_unique();
        }
    };

    static_assert(is_stream_reader_v<HeapStreamReader<uint8_t>, uint8_t>);

    template <typename T>
    etl::pair<HeapStreamWriter<T>, HeapStreamReader<T>> make_heap_stream(size_t capacity) {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer{capacity};
        return etl::make_pair(HeapStreamWriter<T>{buffer}, HeapStreamReader<T>{buffer});
    }
} // namespace nb::stream
