#pragma once

#include "./fields.h"
#include <etl/utility.h>
#include <memory/rc_pool.h>
#include <nb/stream.h>

namespace net::frame {
    template <uint8_t BUFFER_LENGTH>
    struct FrameBuffer {
        etl::array<uint8_t, BUFFER_LENGTH> buffer;
        uint8_t length;
        nb::stream::FixedWritableBufferIndex write_index;

        explicit inline FrameBuffer(uint8_t len) : length{len} {}
    };

    class FrameBufferReference {
        memory::RcPoolCounter *counter_;
        etl::span<uint8_t> buffer_;
        nb::stream::FixedWritableBufferIndex *write_index_;

        FrameBufferReference(
            memory::RcPoolCounter *counter,
            etl::span<uint8_t> buffer,
            nb::stream::FixedWritableBufferIndex *write_index
        )
            : counter_{counter},
              buffer_{buffer},
              write_index_{write_index} {
            counter_->increment();
        }

      public:
        template <uint8_t BUFFER_LENGTH>
        FrameBufferReference(memory::RcPoolCounter *counter, FrameBuffer<BUFFER_LENGTH> *buffer)
            : counter_{counter},
              buffer_{buffer->buffer.data(), buffer->length},
              write_index_{&buffer->write_index} {
            counter_->increment();
        }

        FrameBufferReference() = delete;
        FrameBufferReference(const FrameBufferReference &) = delete;

        FrameBufferReference(FrameBufferReference &&other) {
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            write_index_ = other.write_index_;
            other.counter_ = nullptr;
            other.buffer_ = {};
            other.write_index_ = nullptr;
        }

        FrameBufferReference &operator=(const FrameBufferReference &) = delete;

        FrameBufferReference &operator=(FrameBufferReference &&other) {
            counter_ = other.counter_;
            buffer_ = other.buffer_;
            write_index_ = other.write_index_;
            other.counter_ = nullptr;
            other.buffer_ = {};
            other.write_index_ = nullptr;
            return *this;
        }

        ~FrameBufferReference() {
            if (counter_ != nullptr) {
                counter_->decrement();
            }
        }

        inline FrameBufferReference clone() const {
            return FrameBufferReference{counter_, buffer_, write_index_};
        }

        inline etl::span<uint8_t> span() {
            return buffer_;
        }

        inline const etl::span<uint8_t> span() const {
            return buffer_;
        }

        inline uint8_t written_count() const {
            return write_index_->index();
        }

        inline uint8_t frame_length() const {
            return buffer_.size();
        }

        inline void shrink_frame_length_to_fit() {
            buffer_ = buffer_.first(write_index_->index());
        }

        inline nb::stream::FixedWritableBufferIndex &write_index() {
            return *write_index_;
        }

        inline const nb::stream::FixedWritableBufferIndex &write_index() const {
            return *write_index_;
        }
    };

    template <uint8_t BUFFER_LENGTH, uint8_t BUFFER_COUNT>
    class FrameBufferPool {
        memory::RcPool<FrameBuffer<BUFFER_LENGTH>, BUFFER_COUNT> pool_;

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
            new (buffer) FrameBuffer<BUFFER_LENGTH>{length};
            return FrameBufferReference{counter, buffer};
        }
    };

    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    class FrameBufferAllocator {
        static constexpr uint8_t SHORT_BUFFER_LENGTH = 16;
        static constexpr uint8_t LARGE_BUFFER_LENGTH = MTU;

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

        inline nb::Poll<FrameBufferReference> allocate_max_length() {
            return long_pool_.allocate(LARGE_BUFFER_LENGTH);
        }
    };
}; // namespace net::frame
