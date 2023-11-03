#pragma once

#include "./buffer.h"
#include "./pool.h"
#include "./protocol_number.h"
#include <etl/list.h>
#include <util/concepts.h>

namespace net::frame {
    template <typename Service>
    concept IFrameService = requires(Service &service, uint8_t size) {
        { service.request_frame_writer(size) } -> util::same_as<nb::Poll<FrameBufferWriter>>;

        { service.request_max_length_frame_writer() } -> util::same_as<nb::Poll<FrameBufferWriter>>;
    };

    class FrameService {
        FrameBufferAllocator allocator_;
        uint8_t max_frame_count_;

      public:
        FrameService() = delete;
        FrameService(const FrameService &) = delete;
        FrameService(FrameService &&) = delete;
        FrameService &operator=(const FrameService &) = delete;
        FrameService &operator=(FrameService &&) = delete;

        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        FrameService(
            memory::Static<MultiSizeFrameBufferPool<SHORT_BUFFER_LENGTH, LARGE_BUFFER_COUNT>> &pool,
            uint8_t max_frame_count
        )
            : allocator_{pool.get().allocator()},
              max_frame_count_{max_frame_count} {}

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
