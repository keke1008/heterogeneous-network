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
            : task_executor_{neighbor::NeighborSocket{
                  link_service.open(frame::ProtocolNumber::Discover)
              }},
              discovery_requests_{time} {}

        void execute(
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            const node::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            util::Time &time,
            util::Rand &rand
        ) {
            auto opt_event = task_executor_.execute(fs, ls, lns, ns, discover_cache_, rand);
            if (opt_event) {
                const auto &event = opt_event.value();
                discovery_requests_.on_gateway_found(event.remote_id, event.gateway_id, event.cost);
            }

            discovery_requests_.execute(time, discover_cache_);
        }
    };

    class DiscoveryTask {
        DiscoveryHandler handler_;

      public:
        explicit DiscoveryTask(const node::NodeId &target_id) : handler_{target_id} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::optional<node::NodeId>> execute(
            const node::LocalNodeService &local_node_service,
            neighbor::NeighborService<RW> &neighbor_service,
            DiscoveryService<RW> &discovery_service,
            util::Time &time,
            util::Rand &rand
        ) {
            return handler_.execute(
                local_node_service, neighbor_service, discovery_service.discovery_requests_,
                discovery_service.discover_cache_, discovery_service.task_executor_, time, rand
            );
        }
    };

} // namespace net::discovery
