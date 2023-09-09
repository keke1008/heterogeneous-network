#include "buffer.h"

namespace net::packet {
    PacketBuffer::~PacketBuffer() {
        pool_->release(memory::as_counter_cell(this));
    }

    etl::optional<UninitializedPacketBuffer> PacketBufferPool::allocate() {
        auto buffer = pool_.allocate();
        if (buffer != nullptr) {
            return UninitializedPacketBuffer{this, buffer};
        }
        return etl::nullopt;
    }

    util::Tuple<PacketBufferReader, PacketBufferWriter>
    UninitializedPacketBuffer::initialize(uint8_t max_size) && {
        new (ptr_) memory::CounterCell<PacketBuffer>{pool_, max_size};
        auto rc = memory::Rc<PacketBuffer>{ptr_};
        auto rc_ = rc.clone();
        return util::Tuple{
            PacketBufferReader{etl::move(rc)},
            PacketBufferWriter{etl::move(rc_)},
        };
    }
} // namespace net::packet
