#pragma once

#include "./service.h"

namespace net {
    template <nb::AsyncReadableWritable RW>
    class App {
        memory::Static<BufferPool<4, 2>> buffer_pool_{};
        memory::Static<link::LinkFrameQueue> frame_queue_{};

        etl::vector<memory::Static<link::MediaPort<RW>>, link::MAX_MEDIA_PORT> media_ports_{};
        memory::Static<NetService<RW>> net_service_;

      public:
        explicit App(util::Time &time)
            : net_service_{time, buffer_pool_, media_ports_, frame_queue_} {}

        void add_serial_port(util::Time &time, memory::Static<RW> &serial) {
            media_ports_.emplace_back(serial, time, frame_queue_);
        }

        void execute(util::Time &time, util::Rand &rand) {
            net_service_->execute(time, rand);
        }
    };
} // namespace net
