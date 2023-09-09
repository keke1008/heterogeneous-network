#pragma once

#include <etl/array.h>
#include <etl/optional.h>
#include <etl/pool.h>
#include <memory/rc.h>
#include <nb/stream.h>
#include <stdint.h>
#include <util/tuple.h>

namespace net::packet {
    class PacketBufferPool;

    class PacketBuffer {
        PacketBufferPool *pool_;
        nb::stream::FixedReadableWritableBuffer<255> buffer_;

      public:
        inline PacketBuffer(PacketBufferPool *pool, uint8_t max_size)
            : buffer_{max_size},
              pool_{pool} {}

        PacketBuffer(const PacketBuffer &) = delete;
        PacketBuffer(PacketBuffer &&) = delete;
        PacketBuffer &operator=(const PacketBuffer &) = delete;
        PacketBuffer &operator=(PacketBuffer &&) = delete;

        ~PacketBuffer();

        const nb::stream::FixedReadableWritableBuffer<255> &inner() const {
            return buffer_;
        }

        nb::stream::FixedReadableWritableBuffer<255> &inner() {
            return buffer_;
        }
    };

    class PacketBufferReader final : nb::stream::ReadableStream, nb::stream::ReadableBuffer {
        memory::Rc<PacketBuffer> buffer_;

      public:
        inline PacketBufferReader(memory::Rc<PacketBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

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

    class PacketBufferWriter : nb::stream::WritableStream, nb::stream::WritableBuffer {
        memory::Rc<PacketBuffer> buffer_;

      public:
        inline PacketBufferWriter(memory::Rc<PacketBuffer> &&buffer) : buffer_{etl::move(buffer)} {}

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
