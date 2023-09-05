#include "buffer.h"

namespace net::packet {
    PacketBuffer::~PacketBuffer() {
        pool_->release(memory::as_counter_cell(this));
    }

    etl::optional<util::Tuple<PacketBufferReader, PacketBufferWriter>>
    PacketBufferPool::allocate(const uint8_t max_size) {
        auto buffer = new (pool_.allocate()) memory::CounterCell<PacketBuffer>{this, max_size};
        if (buffer != nullptr) {
            return util::Tuple{PacketBufferReader{buffer}, PacketBufferWriter{buffer}};
        }
        return etl::nullopt;
    }
} // namespace net::packet
