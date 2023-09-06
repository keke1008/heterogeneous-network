#pragma once

#include <etl/array.h>
#include <etl/optional.h>
#include <etl/pool.h>
#include <memory/rc.h>
#include <stdint.h>
#include <util/tuple.h>

namespace net::packet {
    class PacketBufferPool;
    class PacketBuffer;

    class PacketBuffer {
        const uint8_t max_size_;
        uint8_t read_index_ = 0;
        uint8_t write_index_ = 0;
        PacketBufferPool *pool_;
        etl::array<uint8_t, 256> buffer_;

      public:
        inline PacketBuffer(PacketBufferPool *pool, uint8_t max_size)
            : max_size_{max_size},
              pool_{pool} {}

        PacketBuffer(const PacketBuffer &) = delete;
        PacketBuffer(PacketBuffer &&) = delete;
        PacketBuffer &operator=(const PacketBuffer &) = delete;
        PacketBuffer &operator=(PacketBuffer &&) = delete;

        ~PacketBuffer();

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

    class PacketBufferReader {
        memory::Rc<PacketBuffer> buffer_;

      public:
        inline PacketBufferReader(memory::Rc<PacketBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

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

    class PacketBufferWriter {
        memory::Rc<PacketBuffer> buffer_;

      public:
        inline PacketBufferWriter(memory::Rc<PacketBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

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

    class UninitializedPacketBuffer {
        PacketBufferPool *pool_;
        memory::CounterCell<PacketBuffer> *ptr_;

      public:
        UninitializedPacketBuffer() = delete;
        UninitializedPacketBuffer(const UninitializedPacketBuffer &) = delete;
        UninitializedPacketBuffer(UninitializedPacketBuffer &&) = default;
        UninitializedPacketBuffer &operator=(const UninitializedPacketBuffer &) = delete;
        UninitializedPacketBuffer &operator=(UninitializedPacketBuffer &&) = default;

        inline UninitializedPacketBuffer(
            PacketBufferPool *pool,
            memory::CounterCell<PacketBuffer> *ptr
        )
            : pool_{pool},
              ptr_{ptr} {}

        util::Tuple<PacketBufferReader, PacketBufferWriter> initialize(uint8_t max_size) &&;
    };

    class PacketBufferPool {
      public:
        static constexpr uint8_t BUFFER_COUNT = 8;

      private:
        etl::pool<memory::CounterCell<PacketBuffer>, BUFFER_COUNT> pool_;

      public:
        PacketBufferPool() = default;
        PacketBufferPool(const PacketBufferPool &) = delete;
        PacketBufferPool(PacketBufferPool &&) = delete;
        PacketBufferPool &operator=(const PacketBufferPool &) = delete;
        PacketBufferPool &operator=(PacketBufferPool &&) = delete;

        inline void release(memory::CounterCell<PacketBuffer> *buffer) {
            pool_.release(buffer);
        }

        etl::optional<UninitializedPacketBuffer> allocate();
    };
} // namespace net::packet
