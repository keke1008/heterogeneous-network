#pragma once

#include <etl/utility.h>
#include <memory/rc_pool.h>
#include <nb/stream.h>

namespace net::frame {
    class FrameBufferReference {
        memory::RcPoolCounter *counter_;
        nb::stream::IFixedReadableWritableBuffer *buffer_;

      public:
        FrameBufferReference(
            memory::RcPoolCounter *counter,
            nb::stream::IFixedReadableWritableBuffer *buffer
        )
            : counter_{counter},
              buffer_{buffer} {
            counter_->increment();
        }

        FrameBufferReference() = delete;
        FrameBufferReference(const FrameBufferReference &) = delete;

        FrameBufferReference(FrameBufferReference &&other) {
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            other.counter_ = nullptr;
            other.buffer_ = nullptr;
        }

        FrameBufferReference &operator=(const FrameBufferReference &) = delete;

        FrameBufferReference &operator=(FrameBufferReference &&other) {
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            other.counter_ = nullptr;
            other.buffer_ = nullptr;
            return *this;
        }

        ~FrameBufferReference() {
            if (counter_ != nullptr) {
                counter_->decrement();
            }
        }

        inline FrameBufferReference clone() const {
            return FrameBufferReference{counter_, buffer_};
        }

        inline nb::stream::IFixedReadableWritableBuffer &buffer() {
            return *buffer_;
        }

        inline const nb::stream::IFixedReadableWritableBuffer &buffer() const {
            return *buffer_;
        }
    };

    template <uint8_t BUFFER_LENGTH, uint8_t BUFFER_COUNT>
    class FrameBufferPool {
        memory::RcPool<nb::stream::FixedReadableWritableBuffer<BUFFER_LENGTH>, BUFFER_COUNT> pool_;

      public:
        FrameBufferPool() = default;
        FrameBufferPool(const FrameBufferPool &) = delete;
        FrameBufferPool(FrameBufferPool &&) = delete;
        FrameBufferPool &operator=(const FrameBufferPool &) = delete;
        FrameBufferPool &operator=(FrameBufferPool &&) = delete;

        nb::Poll<FrameBufferReference> allocate(uint8_t length) {
            auto result = pool_.allocate();
            if (!result.has_value()) {
                return nb::pending;
            }

            auto [counter, buffer] = result.value();
            new (buffer) nb::stream::FixedReadableWritableBuffer<BUFFER_LENGTH>{length};
            return FrameBufferReference{counter, buffer};
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

        inline nb::Poll<FrameBufferReference> allocate(uint8_t length) {
            if (length <= SHORT_BUFFER_LENGTH) {
                return short_pool_.allocate(length);
            }
            return long_pool_.allocate(length);
        }
    };
}; // namespace net::frame
