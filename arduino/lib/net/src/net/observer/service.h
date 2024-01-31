#pragma once

#include "./notification.h"
#include "./subscribe.h"
#include "./sync.h"

namespace net::observer {
    class ObserverService {
        NotificationService notification_service_;
        SubscribeService subscribe_service_;
        SyncService sync_service_;
        routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> socket_;

      public:
        explicit ObserverService(link::LinkService &link_service, util::Time &time)
            : notification_service_{},
              subscribe_service_{},
              sync_service_{time},
              socket_{link_service.open(frame::ProtocolNumber::Observer), SOCKET_CONFIG} {}

        void execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            neighbor::NeighborService &ns,
            discovery::DiscoveryService &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(fs, ms, lns, ns, ds, time, rand);

            const auto observer = subscribe_service_.observer();
            notification_service_.execute(fs, nts, lns, socket_, time, rand, observer);
            sync_service_.execute(fs, lns, ns, socket_, observer, time, rand);

            this->subscribe_service_.execute(time, socket_);
        }
    };
} // namespace net::observer
