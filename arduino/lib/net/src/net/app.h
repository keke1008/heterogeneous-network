#pragma once

#include "./service.h"
#include <undefArduino.h>

namespace net {
    class App {
        memory::Static<BufferPool<1, 1>> buffer_pool_{};
        memory::Static<link::LinkFrameQueue> frame_queue_{};

        etl::vector<memory::Static<link::MediaPort>, link::MAX_MEDIA_PORT> media_ports_{};
        memory::Static<NetService> net_service_;

      public:
        explicit App(util::Time &time)
            : net_service_{time, buffer_pool_, media_ports_, frame_queue_} {}

        template <typename Serial>
        void add_serial_port(util::Time &time, Serial &serial) {
            nb::stream::SerialStream<Serial> serial_stream{serial};
            media_ports_.emplace_back(serial_stream, time, frame_queue_);
        }

        void execute(util::Time &time, util::Rand &rand) {
            net_service_->execute(time, rand);
        }
    };
} // namespace net
