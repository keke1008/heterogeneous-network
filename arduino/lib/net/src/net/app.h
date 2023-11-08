#pragma once

#include "./service.h"
#include <undefArduino.h>

namespace net {
    // nb::stream::SerialStream<HardwareSerial> serial1{Serial1};
    // util::ArduinoTime time;
    //
    // memory::Static<BufferPool<8, 4>> buffer_pool;
    // memory::Static<link::LinkFrameQueue> frame_queue;
    // memory::Static<link::MediaPort> media_port_1{serial1, time, frame_queue.ref()};
    // etl::span<memory::Static<link::MediaPort>> media_ports{&media_port_1, 1};
    //
    // memory::Static<NetService> net_service{time, buffer_pool.ref(), frame_queue.ref(),
    // media_ports};
} // namespace net
