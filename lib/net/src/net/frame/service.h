#pragma once

#include "./pool.h"
#include <nb/future.h>

namespace net::frame {
    class FrameService {
        FrameBufferAllocator allocator_;

      public:
        static constexpr uint8_t MAX_FRAME_COUNT = FrameBufferAllocator::MAX_FRAME_COUNT;

        FrameService() = default;
        FrameService(const FrameService &) = delete;
        FrameService(FrameService &&) = delete;
        FrameService &operator=(const FrameService &) = delete;
        FrameService &operator=(FrameService &&) = delete;

        nb::Poll<etl::pair<FrameBufferReader, FrameBufferWriter>> allocate(uint8_t max_size) {
            return allocator_.allocate(max_size);
        }
    };
} // namespace net::frame
