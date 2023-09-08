#pragma once

#include <etl/array.h>
#include <etl/optional.h>
#include <etl/pool.h>
#include <memory/rc.h>
#include <stdint.h>
#include <util/tuple.h>

namespace net::link {
    class FrameBufferPool;
    class FrameBuffer;

    class FrameBuffer {
        const uint8_t max_size_;
        uint8_t read_index_ = 0;
        uint8_t write_index_ = 0;
        FrameBufferPool *pool_;
        etl::array<uint8_t, 256> buffer_;

      public:
        inline FrameBuffer(FrameBufferPool *pool, uint8_t max_size)
            : max_size_{max_size},
              pool_{pool} {}

        FrameBuffer(const FrameBuffer &) = delete;
        FrameBuffer(FrameBuffer &&) = delete;
        FrameBuffer &operator=(const FrameBuffer &) = delete;
        FrameBuffer &operator=(FrameBuffer &&) = delete;

        ~FrameBuffer();

        inline constexpr bool is_readable() const {
            return read_index_ < write_index_;
        }

        inline constexpr uint8_t readable_count() const {
            return write_index_ - read_index_;
        }

        inline constexpr etl::optional<uint8_t> read() {
            if (is_readable()) {
                return buffer_[read_index_++];
            }
            return etl::nullopt;
        }

        inline constexpr bool is_reader_closed() const {
            return read_index_ == max_size_;
        }

        inline constexpr bool is_writable() const {
            return write_index_ < max_size_;
        }

        inline constexpr uint8_t writable_count() const {
            return max_size_ - write_index_;
        }

        inline constexpr bool write(uint8_t byte) {
            if (is_writable()) {
                buffer_[write_index_++] = byte;
                return true;
            }
            return false;
        }

        inline constexpr bool is_writer_closed() const {
            return write_index_ == max_size_;
        }
    };

    class FrameBufferReader {
        memory::Rc<FrameBuffer> buffer_;

      public:
        inline FrameBufferReader(memory::Rc<FrameBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

        inline constexpr bool is_readable() const {
            return buffer_.get().is_readable();
        }

        inline constexpr uint8_t readable_count() const {
            return buffer_.get().readable_count();
        }

        inline constexpr etl::optional<uint8_t> read() {
            return buffer_.get().read();
        }

        inline constexpr bool is_reader_closed() const {
            return buffer_.get().is_reader_closed();
        }
    };

    class FrameBufferWriter {
        memory::Rc<FrameBuffer> buffer_;

      public:
        inline FrameBufferWriter(memory::Rc<FrameBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

        inline constexpr bool is_writable() const {
            return buffer_.get().is_writable();
        }

        inline constexpr uint8_t writable_count() const {
            return buffer_.get().writable_count();
        }

        inline constexpr void write(uint8_t byte) {
            buffer_.get().write(byte);
        }

        inline constexpr bool is_writer_closed() const {
            return buffer_.get().is_writer_closed();
        }
    };

    class UninitializedFrameBuffer {
        FrameBufferPool *pool_;
        memory::CounterCell<FrameBuffer> *ptr_;

      public:
        UninitializedFrameBuffer() = delete;
        UninitializedFrameBuffer(const UninitializedFrameBuffer &) = delete;
        UninitializedFrameBuffer(UninitializedFrameBuffer &&) = default;
        UninitializedFrameBuffer &operator=(const UninitializedFrameBuffer &) = delete;
        UninitializedFrameBuffer &operator=(UninitializedFrameBuffer &&) = default;

        inline UninitializedFrameBuffer(
            FrameBufferPool *pool,
            memory::CounterCell<FrameBuffer> *ptr
        )
            : pool_{pool},
              ptr_{ptr} {}

        util::Tuple<FrameBufferReader, FrameBufferWriter> initialize(uint8_t max_size) &&;
    };

    class FrameBufferPool {
      public:
        static constexpr uint8_t BUFFER_COUNT = 8;

      private:
        etl::pool<memory::CounterCell<FrameBuffer>, BUFFER_COUNT> pool_;

      public:
        FrameBufferPool() = default;
        FrameBufferPool(const FrameBufferPool &) = delete;
        FrameBufferPool(FrameBufferPool &&) = delete;
        FrameBufferPool &operator=(const FrameBufferPool &) = delete;
        FrameBufferPool &operator=(FrameBufferPool &&) = delete;

        inline void release(memory::CounterCell<FrameBuffer> *buffer) {
            pool_.release(buffer);
        }

        etl::optional<UninitializedFrameBuffer> allocate();
    };
} // namespace net::link
