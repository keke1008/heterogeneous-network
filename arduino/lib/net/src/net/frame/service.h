// IWYU pragma: private
#pragma once

#include "./buffer.h"
#include "./pool.h"
#include <etl/list.h>
#include <tl/concepts.h>

namespace net::frame {
    class FrameService {
        FrameBufferAllocator allocator_;

      public:
        FrameService() = delete;
        FrameService(const FrameService &) = delete;
        FrameService(FrameService &&) = delete;
        FrameService &operator=(const FrameService &) = delete;
        FrameService &operator=(FrameService &&) = delete;

        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        FrameService(
            memory::Static<MultiSizeFrameBufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>> &pool
        )
            : allocator_{pool->allocator()} {}

        nb::Poll<FrameBufferWriter> request_frame_writer(uint8_t length) {
            auto buffer_ref = POLL_MOVE_UNWRAP_OR_RETURN(allocator_.allocate(length));
            return FrameBufferWriter{etl::move(buffer_ref)};
        }

        nb::Poll<FrameBufferWriter> request_max_length_frame_writer() {
            auto buffer_ref = POLL_MOVE_UNWRAP_OR_RETURN(allocator_.allocate_max_length());
            return FrameBufferWriter{etl::move(buffer_ref)};
        }
    };
} // namespace net::frame
