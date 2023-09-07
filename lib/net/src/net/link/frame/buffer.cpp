#include "buffer.h"

namespace net::link {
    FrameBuffer::~FrameBuffer() {
        pool_->release(memory::as_counter_cell(this));
    }

    etl::optional<UninitializedFrameBuffer> FrameBufferPool::allocate() {
        auto buffer = pool_.allocate();
        if (buffer != nullptr) {
            return UninitializedFrameBuffer{this, buffer};
        }
        return etl::nullopt;
    }

    util::Tuple<FrameBufferReader, FrameBufferWriter>
    UninitializedFrameBuffer::initialize(uint8_t max_size) && {
        new (ptr_) memory::CounterCell<FrameBuffer>{pool_, max_size};
        auto rc = memory::Rc<FrameBuffer>{ptr_};
        auto rc_ = rc.clone();
        return util::Tuple{
            FrameBufferReader{etl::move(rc)},
            FrameBufferWriter{etl::move(rc_)},
        };
    }
} // namespace net::link
