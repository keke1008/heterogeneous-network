#pragma once

#include "./discovery.h"
#include "./frame.h"
#include "./link.h"
#include "./neighbor.h"
#include "./notification.h"
#include "./observer.h"
#include "./rpc.h"
#include "./tunnel.h"
#include <util/time.h>

namespace net {
    template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
    using BufferPool = frame::MultiSizeFrameBufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>;

    class NetService {
        link::LinkService link_service_;
        notification::NotificationService notification_service_;
        local::LocalNodeService local_node_service_;
        neighbor::NeighborService neighbor_service_;
        discovery::DiscoveryService discovery_service_;
        rpc::RpcService rpc_service_;
        observer::ObserverService observer_service_;
        tunnel::TunnelService tunnel_service_;

      public:
        explicit NetService(
            util::Time &time,
            memory::Static<link::MeasuredLinkFrameQueue> &frame_queue
        )
            : link_service_{frame_queue},
              notification_service_{},
              local_node_service_{},
              neighbor_service_{link_service_, time},
              discovery_service_{link_service_, time},
              rpc_service_{link_service_},
              observer_service_{link_service_},
              tunnel_service_{link_service_} {}

        inline void execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            util::Time &time,
            util::Rand &rand
        ) {
            local_node_service_.execute(ms, notification_service_);
            neighbor_service_.execute(fs, ms, local_node_service_, notification_service_, time);
            discovery_service_.execute(fs, ms, local_node_service_, neighbor_service_, time, rand);
            rpc_service_.execute(
                fs, ms, link_service_, notification_service_, local_node_service_,
                neighbor_service_, discovery_service_, time, rand
            );
            observer_service_.execute(
                fs, ms, local_node_service_, notification_service_, neighbor_service_,
                discovery_service_, time, rand
            );
            tunnel_service_.execute(
                fs, ms, local_node_service_, notification_service_, neighbor_service_,
                discovery_service_, time, rand
            );
        }
    };
} // namespace net
