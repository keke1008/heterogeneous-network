#pragma once

#include "./fields.h"
#include <etl/utility.h>
#include <memory/rc_pool.h>
#include <nb/serde.h>

namespace net::frame {
    template <uint8_t BUFFER_LENGTH>
    struct FrameBuffer {
        etl::array<uint8_t, BUFFER_LENGTH> buffer;
        uint8_t length;
        uint8_t written_index;

        explicit inline FrameBuffer(uint8_t len) : length{len} {}
    };

    class VariadicFrameBuffer {
        etl::span<uint8_t> buffer_;
        uint8_t *written_index_;
        uint8_t begin_index_;
        uint8_t length_;

        VariadicFrameBuffer(
            etl::span<uint8_t> buffer,
            uint8_t *written_index,
            uint8_t begin_index,
            uint8_t length
        )
            : buffer_{buffer},
              written_index_{written_index},
              begin_index_{begin_index},
              length_{length} {}

      public:
        VariadicFrameBuffer() = delete;
        VariadicFrameBuffer(const VariadicFrameBuffer &) = delete;

        VariadicFrameBuffer(VariadicFrameBuffer &&other)
            : buffer_{other.buffer_},
              written_index_{other.written_index_},
              begin_index_{other.begin_index_},
              length_{other.length_} {
            other.buffer_ = {};
            other.written_index_ = nullptr;
            other.begin_index_ = 0;
            other.length_ = 0;
        }

        VariadicFrameBuffer &operator=(const VariadicFrameBuffer &) = delete;

        VariadicFrameBuffer &operator=(VariadicFrameBuffer &&other) {
            buffer_ = other.buffer_;
            written_index_ = other.written_index_;
            begin_index_ = other.begin_index_;
            length_ = other.length_;
            other.buffer_ = {};
            other.written_index_ = nullptr;
            other.begin_index_ = 0;
            other.length_ = 0;
            return *this;
        }

        template <uint8_t BUFFER_LENGTH>
        VariadicFrameBuffer(FrameBuffer<BUFFER_LENGTH> &buffer)
            : buffer_{buffer.buffer.data(), buffer.length},
              written_index_{&buffer.written_index},
              begin_index_{0},
              length_{buffer.length} {}

        static VariadicFrameBuffer empty() {
            return VariadicFrameBuffer{etl::span<uint8_t>{}, nullptr, 0, 0};
        }

        inline VariadicFrameBuffer origin() const {
            uint8_t origin_length = static_cast<uint8_t>(buffer_.size());
            return VariadicFrameBuffer{buffer_, written_index_, 0, origin_length};
        }

        inline VariadicFrameBuffer clone() const {
            return VariadicFrameBuffer{buffer_, written_index_, begin_index_, length_};
        }

        inline VariadicFrameBuffer slice(uint8_t offset) const {
            uint8_t new_begin_index = begin_index_ + offset;
            FASSERT(new_begin_index <= buffer_.size());

            uint8_t new_length = length_ - offset;
            return VariadicFrameBuffer{buffer_, written_index_, new_begin_index, new_length};
        }

        inline VariadicFrameBuffer slice(uint8_t offset, uint8_t length) const {
            uint8_t new_begin_index = begin_index_ + offset;
            FASSERT(new_begin_index <= buffer_.size());

            uint8_t new_length = length;
            FASSERT(new_begin_index + new_length <= buffer_.size());

            return VariadicFrameBuffer{buffer_, written_index_, new_begin_index, new_length};
        }

        inline uint8_t written_index() const {
            if (written_index_ == nullptr) {
                return 0;
            }
            if (*written_index_ < begin_index_) {
                return 0;
            }
            return *written_index_ - begin_index_;
        }

        inline const etl::span<const uint8_t> written_buffer() const {
            return buffer_.subspan(begin_index_, written_index());
        }

        inline uint8_t buffer_length() const {
            return length_;
        }

        inline uint8_t origin_length() const {
            return static_cast<uint8_t>(buffer_.size());
        }

        inline bool is_all_written() const {
            return written_index() == length_;
        }

        inline void write_unchecked(uint8_t byte) {
            buffer_[(*written_index_)++] = byte;
        }
    };

    class FrameBufferReference {
        memory::RcPoolCounter *counter_;
        VariadicFrameBuffer buffer_;

        FrameBufferReference(memory::RcPoolCounter *counter, VariadicFrameBuffer &&buffer)
            : counter_{counter},
              buffer_{etl::move(buffer)} {
            counter_->increment();
        }

      public:
        template <uint8_t BUFFER_LENGTH>
        FrameBufferReference(memory::RcPoolCounter *counter, FrameBuffer<BUFFER_LENGTH> *buffer)
            : FrameBufferReference{counter, VariadicFrameBuffer{*buffer}} {}

        FrameBufferReference() = delete;
        FrameBufferReference(const FrameBufferReference &) = delete;

        FrameBufferReference(FrameBufferReference &&other)
            : counter_{other.counter_},
              buffer_{etl::move(other.buffer_)} {
            other.counter_ = nullptr;
        }

        FrameBufferReference &operator=(const FrameBufferReference &) = delete;

        FrameBufferReference &operator=(FrameBufferReference &&other) {
            counter_ = other.counter_;
            buffer_ = etl::move(other.buffer_);
            other.counter_ = nullptr;
            return *this;
        }

        ~FrameBufferReference() {
            if (counter_ != nullptr) {
                counter_->decrement();
            }
        }

        static FrameBufferReference empty() {
            return FrameBufferReference{nullptr, VariadicFrameBuffer::empty()};
        }

        inline FrameBufferReference origin() const {
            return FrameBufferReference{counter_, buffer_.origin()};
        }

        inline FrameBufferReference clone() const {
            return FrameBufferReference{counter_, buffer_.clone()};
        }

        inline FrameBufferReference slice(uint8_t offset) const {
            return FrameBufferReference{counter_, buffer_.slice(offset)};
        }

        inline uint8_t written_index() const {
            return buffer_.written_index();
        }

        inline const etl::span<const uint8_t> written_buffer() const {
            return buffer_.written_buffer();
        }

        inline uint8_t buffer_length() const {
            return buffer_.buffer_length();
        }

        inline uint8_t origin_length() const {
            return buffer_.origin_length();
        }

        inline bool is_all_written() const {
            return buffer_.is_all_written();
        }

        inline void write_unchecked(uint8_t byte) {
            buffer_.write_unchecked(byte);
        }
    };

    template <uint8_t BUFFER_LENGTH>
    class FrameBufferPoolReference {
        memory::RcPoolRef<FrameBuffer<BUFFER_LENGTH>> ipool_;

      public:
        FrameBufferPoolReference() = delete;
        FrameBufferPoolReference(const FrameBufferPoolReference &) = default;
        FrameBufferPoolReference(FrameBufferPoolReference &&) = default;
        FrameBufferPoolReference &operator=(const FrameBufferPoolReference &) = default;
        FrameBufferPoolReference &operator=(FrameBufferPoolReference &&) = default;

        template <uint8_t BUFFER_COUNT>
        explicit FrameBufferPoolReference(
            memory::Static<memory::RcPool<FrameBuffer<BUFFER_LENGTH>, BUFFER_COUNT>> &pool
        )
            : ipool_{pool} {}

        nb::Poll<FrameBufferReference> allocate(uint8_t length) {
            auto result = ipool_.allocate();
            if (!result.has_value()) {
                return nb::pending;
            }

            auto [counter, buffer] = result.value();
            new (buffer) FrameBuffer<BUFFER_LENGTH>{length};
            return FrameBufferReference{counter, buffer};
        }
    };

    static constexpr uint8_t SHORT_BUFFER_LENGTH = 16;
    static constexpr uint8_t LARGE_BUFFER_LENGTH = MTU;

    class FrameBufferAllocator {
        FrameBufferPoolReference<SHORT_BUFFER_LENGTH> short_pool_ref_;
        FrameBufferPoolReference<LARGE_BUFFER_LENGTH> large_pool_ref_;

      public:
        FrameBufferAllocator() = delete;
        FrameBufferAllocator(const FrameBufferAllocator &) = default;
        FrameBufferAllocator(FrameBufferAllocator &&) = default;
        FrameBufferAllocator &operator=(const FrameBufferAllocator &) = delete;
        FrameBufferAllocator &operator=(FrameBufferAllocator &&) = delete;

        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        FrameBufferAllocator(
            memory::Static<memory::RcPool<FrameBuffer<SHORT_BUFFER_LENGTH>, SHORT_BUFFER_COUNT>>
                &short_pool,
            memory::Static<memory::RcPool<FrameBuffer<LARGE_BUFFER_LENGTH>, LARGE_BUFFER_COUNT>>
                &large_pool
        )
            : short_pool_ref_{short_pool},
              large_pool_ref_{large_pool} {}

        inline nb::Poll<FrameBufferReference> allocate(uint8_t length) {
            if (length == 0) {
                return FrameBufferReference::empty();
            }
            if (length <= SHORT_BUFFER_LENGTH) {
                return short_pool_ref_.allocate(length);
            }
            return large_pool_ref_.allocate(length);
        }

        inline nb::Poll<FrameBufferReference> allocate_max_length() {
            return large_pool_ref_.allocate(LARGE_BUFFER_LENGTH);
        }
    };

    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    class MultiSizeFrameBufferPool {
        memory::Static<memory::RcPool<FrameBuffer<SHORT_BUFFER_LENGTH>, SHORT_BUFFER_COUNT>>
            short_pool_;
        memory::Static<memory::RcPool<FrameBuffer<LARGE_BUFFER_LENGTH>, LARGE_BUFFER_COUNT>>
            large_pool_;

      public:
        static constexpr uint8_t MAX_FRAME_COUNT = SHORT_BUFFER_COUNT + LARGE_BUFFER_COUNT;

        MultiSizeFrameBufferPool() = default;
        MultiSizeFrameBufferPool(const MultiSizeFrameBufferPool &) = delete;
        MultiSizeFrameBufferPool(MultiSizeFrameBufferPool &&) = delete;
        MultiSizeFrameBufferPool &operator=(const MultiSizeFrameBufferPool &) = delete;
        MultiSizeFrameBufferPool &operator=(MultiSizeFrameBufferPool &&) = delete;

        inline FrameBufferAllocator allocator() {
            return FrameBufferAllocator{short_pool_, large_pool_};
        }
    };

}; // namespace net::frame
