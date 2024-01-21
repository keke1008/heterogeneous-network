#pragma once

#include <media.h>
#include <net.h>

template <nb::AsyncReadableWritable RW>
class App {
    memory::Static<media::MediaService<RW>> media_service_{};
    memory::Static<net::BufferPool<12, 4>> buffer_pool_{};
    memory::Static<net::link::LinkFrameQueue> frame_queue_;
    memory::Static<net::frame::FrameService> frame_service_{buffer_pool_};
    memory::Static<net::NetService> net_service_;

  public:
    explicit App(util::Time &time) : frame_queue_{time}, net_service_{time, frame_queue_} {}

    inline memory::Static<net::link::LinkFrameQueue> &frame_queue() {
        return frame_queue_;
    }

    template <typename T>
    void register_port(memory::Static<T> &port) {
        media_service_->register_port(port);
    }

    inline void execute(util::Time &time, util::Rand &rand) {
        media_service_->execute(frame_service_.get(), time, rand);
        net_service_->execute(frame_service_.get(), media_service_.get(), time, rand);
        frame_queue_->execute(time);
    }
};
