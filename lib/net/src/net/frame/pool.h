#pragma once

#include <etl/pool.h>
#include <etl/utility.h>
#include <nb/stream.h>

namespace net::frame {
    class Counter {
        uint8_t count_;

      public:
        Counter(uint8_t count) : count_{count} {};

        inline void increment() {
            count_++;
        }

        inline void decrement() {
            count_--;
        }

        inline bool is_zero() const {
            return count_ == 0;
        }
    };

    template <uint8_t SIZE>
    class FrameBuffer {
        friend class FrameBufferReference;

        Counter reference_counter_{0};
        nb::stream::FixedReadableWritableBuffer<SIZE> buffer_;

      public:
        FrameBuffer() = default;
        FrameBuffer(const FrameBuffer &) = delete;
        FrameBuffer(FrameBuffer &&) = delete;
        FrameBuffer &operator=(const FrameBuffer &) = delete;
        FrameBuffer &operator=(FrameBuffer &&) = delete;
    };

    class FrameBufferReference {
        etl::ipool *pool_;
        Counter *counter_;
        nb::stream::IFixedReadableWritableBuffer *buffer_;

        FrameBufferReference(
            etl::ipool *pool,
            Counter *counter,
            nb::stream::IFixedReadableWritableBuffer *buffer
        )
            : pool_{pool},
              counter_{counter},
              buffer_{buffer} {
            counter_->increment();
        }

      public:
        FrameBufferReference() = delete;
        FrameBufferReference(const FrameBufferReference &) = delete;

        FrameBufferReference(FrameBufferReference &&other) {
            pool_ = other.pool_;
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            other.pool_ = nullptr;
            other.counter_ = nullptr;
            other.buffer_ = nullptr;
        }

        FrameBufferReference &operator=(const FrameBufferReference &) = delete;

        FrameBufferReference &operator=(FrameBufferReference &&other) {
            pool_ = other.pool_;
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            other.pool_ = nullptr;
            other.counter_ = nullptr;
            other.buffer_ = nullptr;
            return *this;
        }

        template <uint8_t SIZE>
        explicit FrameBufferReference(etl::ipool *pool, FrameBuffer<SIZE> &buffer)
            : FrameBufferReference{pool, &buffer.reference_counter_, &buffer.buffer_} {}

        ~FrameBufferReference() {
            if (counter_ != nullptr) {
                counter_->decrement();
                if (counter_->is_zero()) {
                    pool_->destroy(this);
                }
            }
        }

        inline FrameBufferReference clone() const {
            counter_->increment();
            return FrameBufferReference{pool_, counter_, buffer_};
        }

        inline nb::stream::IFixedReadableWritableBuffer &buffer() {
            return *buffer_;
        }

        inline const nb::stream::IFixedReadableWritableBuffer &buffer() const {
            return *buffer_;
        }
    };

    class FrameBufferReader final : public nb::stream::ReadableStream,
                                    public nb::stream::ReadableBuffer {
        FrameBufferReference buffer_ref_;

      public:
        FrameBufferReader() = delete;
        FrameBufferReader(const FrameBufferReader &) = delete;
        FrameBufferReader(FrameBufferReader &&) = default;
        FrameBufferReader &operator=(const FrameBufferReader &) = delete;
        FrameBufferReader &operator=(FrameBufferReader &&) = default;

        template <uint8_t SIZE>
        FrameBufferReader(etl::ipool *pool, FrameBuffer<SIZE> &buffer)
            : buffer_ref_{pool, buffer} {}

        inline uint8_t readable_count() const override {
            return buffer_ref_.buffer().readable_count();
        }

        inline uint8_t read() override {
            return buffer_ref_.buffer().read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            buffer_ref_.buffer().read(buffer);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return buffer_ref_.buffer().read_all_into(destination);
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.buffer().length();
        }
    };

    class FrameBufferWriter final : public nb::stream::WritableStream,
                                    public nb::stream::WritableBuffer {
        FrameBufferReference buffer_ref_;

      public:
        FrameBufferWriter() = delete;
        FrameBufferWriter(const FrameBufferWriter &) = delete;
        FrameBufferWriter(FrameBufferWriter &&) = default;
        FrameBufferWriter &operator=(const FrameBufferWriter &) = delete;
        FrameBufferWriter &operator=(FrameBufferWriter &&) = default;

        template <uint8_t SIZE>
        FrameBufferWriter(etl::ipool *pool, FrameBuffer<SIZE> &buffer)
            : buffer_ref_{pool, buffer} {}

        inline uint8_t writable_count() const override {
            return buffer_ref_.buffer().writable_count();
        }

        inline bool write(uint8_t byte) override {
            return buffer_ref_.buffer().write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return buffer_ref_.buffer().write(buffer);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return buffer_ref_.buffer().write_all_from(source);
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.buffer().length();
        }
    };

    template <uint8_t BUFFER_LENGTH, uint8_t BUFFER_COUNT>
    class FrameBufferPool {
        etl::pool<FrameBuffer<BUFFER_LENGTH>, BUFFER_COUNT> pool_;

      public:
        FrameBufferPool() = default;
        FrameBufferPool(const FrameBufferPool &) = delete;
        FrameBufferPool(FrameBufferPool &&) = delete;
        FrameBufferPool &operator=(const FrameBufferPool &) = delete;
        FrameBufferPool &operator=(FrameBufferPool &&) = delete;

        nb::Poll<etl::pair<FrameBufferReader, FrameBufferWriter>> allocate() {
            if (pool_.full()) {
                return nb::pending;
            }
            auto buffer = pool_.create();
            return etl::make_pair(
                FrameBufferReader{&pool_, *buffer}, FrameBufferWriter{&pool_, *buffer}
            );
        }
    };

    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    class FrameBufferAllocator {
        static constexpr uint8_t SHORT_BUFFER_LENGTH = 16;
        static constexpr uint8_t LARGE_BUFFER_LENGTH = 255;

        FrameBufferPool<SHORT_BUFFER_LENGTH, SHORT_BUFFER_COUNT> short_pool_;
        FrameBufferPool<LARGE_BUFFER_LENGTH, LARGE_BUFFER_COUNT> long_pool_;

      public:
        static constexpr uint8_t MAX_FRAME_COUNT = SHORT_BUFFER_COUNT + LARGE_BUFFER_COUNT;

        FrameBufferAllocator() = default;
        FrameBufferAllocator(const FrameBufferAllocator &) = delete;
        FrameBufferAllocator(FrameBufferAllocator &&) = delete;
        FrameBufferAllocator &operator=(const FrameBufferAllocator &) = delete;
        FrameBufferAllocator &operator=(FrameBufferAllocator &&) = delete;

        inline nb::Poll<etl::pair<FrameBufferReader, FrameBufferWriter>> allocate(uint8_t length) {
            if (length <= SHORT_BUFFER_LENGTH) {
                return short_pool_.allocate();
            }
            return long_pool_.allocate();
        }
    };
}; // namespace net::frame
