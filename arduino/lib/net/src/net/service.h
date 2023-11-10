#pragma once

#include "./frame.h"
#include "./link.h"
#include "./routing.h"
#include <util/time.h>

namespace net {
    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    using BufferPool = frame::MultiSizeFrameBufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>;

    class NetService {
        frame::FrameService frame_service_;
        link::LinkService link_service_;
        routing::RoutingService routing_service_;

      public:
        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        explicit NetService(
            util::Time &time,
            memory::Static<BufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>> &buffer_pool,
            etl::vector<memory::Static<link::MediaPort>, link::MAX_MEDIA_PORT> &media_ports,
            memory::Static<link::LinkFrameQueue> &frame_queue
        )
            : frame_service_{buffer_pool},
              link_service_{media_ports, frame_queue},
              routing_service_{link_service_, time} {}

        void execute(util::Time &time, util::Rand &rand) {
            link_service_.execute(frame_service_, time, rand);
            routing_service_.execute(frame_service_, link_service_, rand);
        }
    };
} // namespace net
