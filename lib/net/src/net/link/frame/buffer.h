#pragma once

#include <etl/array.h>
#include <etl/optional.h>
#include <etl/pool.h>
#include <memory/rc.h>
#include <nb/stream.h>
#include <stdint.h>
#include <util/tuple.h>

namespace net::link {
    class FrameBufferPool;

    class FrameBuffer {
        FrameBufferPool *pool_;
        nb::stream::FixedReadableWritableBuffer<255> buffer_;

      public:
        inline FrameBuffer(FrameBufferPool *pool, uint8_t max_size)
            : buffer_{max_size},
              pool_{pool} {}

        FrameBuffer(const FrameBuffer &) = delete;
        FrameBuffer(FrameBuffer &&) = delete;
        FrameBuffer &operator=(const FrameBuffer &) = delete;
        FrameBuffer &operator=(FrameBuffer &&) = delete;

        ~FrameBuffer();

        const nb::stream::FixedReadableWritableBuffer<255> &inner() const {
            return buffer_;
        }

        nb::stream::FixedReadableWritableBuffer<255> &inner() {
            return buffer_;
        }
    };

    class FrameBufferReader final : nb::stream::ReadableStream, nb::stream::ReadableBuffer {
        memory::Rc<FrameBuffer> buffer_;

      public:
        inline FrameBufferReader(memory::Rc<FrameBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

        inline uint8_t readable_count() const override {
            return buffer_.get().inner().readable_count();
        }

        inline uint8_t read() override {
            return buffer_.get().inner().read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            buffer_.get().inner().read(buffer);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return buffer_.get().inner().read_all_into(destination);
        }
    };

    class FrameBufferWriter : nb::stream::WritableStream, nb::stream::WritableBuffer {
        memory::Rc<FrameBuffer> buffer_;

      public:
        inline FrameBufferWriter(memory::Rc<FrameBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

        inline uint8_t writable_count() const override {
            return buffer_.get().inner().writable_count();
        }

        inline bool write(uint8_t byte) override {
            return buffer_.get().inner().write(byte);
        }

        inline bool write(etl::span<uint8_t> buffer) override {
            return buffer_.get().inner().write(buffer);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return buffer_.get().inner().write_all_from(source);
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
