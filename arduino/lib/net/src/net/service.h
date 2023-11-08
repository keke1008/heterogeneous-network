#pragma once

#include "./frame.h"
#include "./link.h"
#include "./routing.h"
#include <util/time.h>

namespace net {
    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    using BufferPool = frame::MultiSizeFrameBufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>;

    class NetService {
        frame::FrameService &frame_service_;
        link::LinkService &link_service_;
        routing::RoutingService &routing_service_;

      public:
        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        explicit NetService(
            util::Time &time,
            memory::StaticRef<BufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>> &buffer_pool,
            memory::StaticRef<link::LinkFrameQueue> &frame_queue,
            etl::span<memory::StaticRef<link::MediaPort>> media_ports
        )
            : frame_service_{buffer_pool},
              link_service_{media_ports, frame_queue},
              routing_service_{link_service_, time} {}
    };
} // namespace net
