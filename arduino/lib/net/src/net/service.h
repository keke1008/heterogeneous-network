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

    template <nb::AsyncReadableWritable RW>
    class NetService {
        frame::FrameService frame_service_;
        link::LinkService<RW> link_service_;
        notification::NotificationService notification_service_;
        local::LocalNodeService local_node_service_;
        neighbor::NeighborService<RW> neighbor_service_;
        discovery::DiscoveryService<RW> discovery_service_;
        rpc::RpcService<RW> rpc_service_;
        observer::ObserverService<RW> observer_service_;
        tunnel::TunnelService<RW> tunnel_service_;

      public:
        template <uint8_t SHORT_BUFFER_COUNT, uint8_t LARGE_BUFFER_COUNT>
        explicit NetService(
            util::Time &time,
            memory::Static<BufferPool<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT>> &buffer_pool,
            etl::vector<memory::Static<link::MediaPort<RW>>, link::MAX_MEDIA_PORT> &media_ports,
            memory::Static<link::LinkFrameQueue> &frame_queue
        )
            : frame_service_{buffer_pool},
              link_service_{media_ports, frame_queue},
              notification_service_{},
              local_node_service_{},
              neighbor_service_{link_service_},
              discovery_service_{link_service_, time},
              rpc_service_{link_service_},
              observer_service_{link_service_},
              tunnel_service_{link_service_} {}

        void execute(util::Time &time, util::Rand &rand) {
            link_service_.execute(frame_service_, time, rand);
            local_node_service_.execute(link_service_);
            neighbor_service_.execute(
                frame_service_, local_node_service_, notification_service_, time
            );
            discovery_service_.execute(
                frame_service_, link_service_, local_node_service_, neighbor_service_, time, rand
            );
            rpc_service_.execute(
                frame_service_, link_service_, notification_service_, local_node_service_,
                neighbor_service_, discovery_service_, time, rand
            );
            observer_service_.execute(
                frame_service_, local_node_service_, notification_service_, neighbor_service_,
                discovery_service_, time, rand
            );
            tunnel_service_.execute(
                frame_service_, local_node_service_, notification_service_, neighbor_service_,
                discovery_service_, time, rand
            );
        }
    };
} // namespace net
