#pragma once

#include "stream.h"
#include <etl/utility.h>
#include <etl/vector.h>
#include <memory/shared.h>
#include <memory/unidirectional_buffer.h>
#include <stddef.h>

namespace nb::stream {
    template <typename T>
    class HeapStreamReader;

    template <typename T>
    class HeapStreamWriter {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer_;

      public:
        using StreamWriterItem = T;

        HeapStreamWriter(const HeapStreamWriter &) = delete;
        HeapStreamWriter &operator=(const HeapStreamWriter &) = delete;

        HeapStreamWriter(HeapStreamWriter &&other) : buffer_{other.buffer_} {}

        HeapStreamWriter &operator=(HeapStreamWriter &&other) {
            buffer_ = other.buffer_;
            return *this;
        }

        HeapStreamWriter(memory::Shared<memory::UnidirectionalBuffer<T>> buffer)
            : buffer_{buffer} {}

        HeapStreamWriter(size_t capacity)
            : buffer_{memory::Shared{memory::UnidirectionalBuffer<T>(capacity)}} {}

        inline bool write(const T item) {
            return buffer_->push(item);
        }

        inline size_t writable_count() const {
            return buffer_->writable_count();
        }

        inline bool is_writable() const {
            return buffer_->writable_count() > 0;
        }

        inline bool is_writer_closed() const {
            return buffer_.is_unique();
        }

        inline HeapStreamReader<T> make_reader() const {
            return HeapStreamReader<T>{buffer_};
        }
    };

    static_assert(is_stream_writer_v<HeapStreamWriter<uint8_t>>);

    template <typename T>
    class HeapStreamReader {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer_;

      public:
        using StreamReaderItem = T;

        HeapStreamReader(const HeapStreamReader &) = delete;
        HeapStreamReader &operator=(const HeapStreamReader &) = delete;

        HeapStreamReader(HeapStreamReader &&other) : buffer_{other.buffer_} {}

        HeapStreamReader &operator=(HeapStreamReader &&other) {
            buffer_ = other.buffer_;
            return *this;
        }

        HeapStreamReader(memory::Shared<memory::UnidirectionalBuffer<T>> buffer)
            : buffer_{buffer} {}

        inline etl::optional<T> read() {
            return buffer_->shift();
        }

        inline size_t readable_count() const {
            return buffer_->readable_count();
        }

        inline bool is_readable() const {
            return buffer_->readable_count() > 0;
        }

        inline bool is_reader_closed() const {
            return buffer_.is_unique();
        }

        inline HeapStreamWriter<T> make_writer() const {
            return HeapStreamWriter<T>{buffer_};
        }
    };

    static_assert(is_stream_reader_v<HeapStreamReader<uint8_t>>);

    template <typename T>
    etl::pair<HeapStreamWriter<T>, HeapStreamReader<T>> make_heap_stream(size_t capacity) {
        memory::Shared<memory::UnidirectionalBuffer<T>> buffer{capacity};
        return etl::make_pair(HeapStreamWriter<T>{buffer}, HeapStreamReader<T>{buffer});
    }
} // namespace nb::stream
