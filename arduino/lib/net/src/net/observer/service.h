#pragma once

#include "./notification.h"
#include "./subscribe.h"

namespace net::observer {
    template <nb::AsyncReadableWritable RW>
    class ObserverService {
        NotificationService notification_service_;
        SubscribeService subscribe_service_;
        routing::RoutingSocket<RW, FRAME_DELAY_POOL_SIZE> socket_;

      public:
        explicit ObserverService(link::LinkService<RW> &link_service)
            : notification_service_{},
              subscribe_service_{},
              socket_{
                  link_service.open(frame::ProtocolNumber::Observer),
                  neighbor::NeighborSocketConfig{.do_delay = false}
              } {}

        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(fs, lns, ns, ds, time, rand);
            notification_service_.execute(
                fs, lns, nts, socket_, time, rand, subscribe_service_.observer()
            );

            this->subscribe_service_.execute(time, socket_);
        }
    };
} // namespace net::observer
