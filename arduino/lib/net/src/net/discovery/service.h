#pragma once

#include "./cache.h"
#include "./discovery.h"
#include "./task.h"
#include <net/neighbor.h>

namespace net::discovery {
    template <nb::AsyncReadableWritable RW>
    class DiscoveryService {
        friend class DiscoveryTask;

        TaskExecutor<RW> task_executor_;
        DiscoveryCache discover_cache_;
        DiscoveryRequests discovery_requests_;

        void handle_received_frame(link::LinkFrame &&frame) {}

      public:
        explicit DiscoveryService(link::LinkService<RW> &link_service, util::Time &time)
            : task_executor_{neighbor::NeighborSocket<RW, FRAME_DELAY_POOL_SIZE>{
                  link_service.open(frame::ProtocolNumber::Discover), SOCKET_CONFIG
              }},
              discover_cache_{time},
              discovery_requests_{time} {}

        void execute(
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            util::Time &time,
            util::Rand &rand
        ) {
            discover_cache_.execute(time);

            auto &poll_info = lns.poll_info();
            if (poll_info.is_pending()) {
                return;
            }

            etl::optional<DiscoveryEvent> opt_event = task_executor_.execute(
                fs, lns, ns, discover_cache_, poll_info.unwrap(), time, rand
            );
            if (opt_event) {
                const auto &event = opt_event.value();
                discovery_requests_.on_gateway_found(
                    time, event.destination, event.gateway_id, event.total_cost
                );
            }

            discovery_requests_.execute(time);
        }
    };

    class DiscoveryTask {
        DiscoveryHandler handler_;

      public:
        explicit DiscoveryTask(const node::Destination &destination) : handler_{destination} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::optional<node::NodeId>> execute(
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            return handler_.execute(
                lns, ns, ds.discovery_requests_, ds.discover_cache_, ds.task_executor_, time, rand
            );
        }
    };

} // namespace net::discovery
