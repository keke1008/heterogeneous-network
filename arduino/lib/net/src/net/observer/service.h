#pragma once

#include "./frame.h"
#include "./notification.h"
#include "./subscribe.h"

namespace net::observer {
    template <nb::AsyncReadableWritable RW>
    class ObserverService {
        NotificationService notification_service_;
        SubscribeService subscribe_service_;
        routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE> socket_;

      public:
        explicit ObserverService(link::LinkService<RW> &link_service)
            : notification_service_{},
              subscribe_service_{},
              socket_{link_service.open(frame::ProtocolNumber::Observer)} {}

        void execute(
            frame::FrameService &fs,
            notification::NotificationService &ns,
            routing::RoutingService<RW> &rs,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(rs, time, rand);
            notification_service_.execute(
                fs, ns, rs, socket_, time, rand, subscribe_service_.observer_id()
            );

            this->subscribe_service_.execute(time, socket_);
        }
    };
} // namespace net::observer
