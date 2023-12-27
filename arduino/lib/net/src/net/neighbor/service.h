#pragma once

#include "./service/task.h"
#include "./table.h"
#include <nb/poll.h>
#include <net/frame.h>
#include <net/link.h>
#include <net/local.h>
#include <net/notification.h>

namespace net::neighbor {
    template <nb::AsyncReadableWritable RW>
    class NeighborService {
        NeighborList neighbor_list_;
        service::TaskExecutor<RW, MAX_NEIGNBOR_FRAME_DELAY_POOL_SIZE> task_executor_;

      public:
        explicit NeighborService(link::LinkService<RW> &link_service)
            : task_executor_{link_service.open(frame::ProtocolNumber::RoutingNeighbor)} {}

        inline etl::optional<node::Cost> get_link_cost(const node::NodeId &neighbor_id) const {
            return neighbor_list_.get_link_cost(neighbor_id);
        }

        inline bool has_neighbor(const node::NodeId &neighbor_id) const {
            return neighbor_list_.has_neighbor_node(neighbor_id);
        }

        inline etl::optional<etl::span<const link::Address>>
        get_address_of(const node::NodeId &node_id) const {
            return neighbor_list_.get_addresses_of(node_id);
        }

        inline nb::Poll<NeighborListCursor> poll_cursor() {
            return neighbor_list_.poll_cursor();
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_neighbor_node(const NeighborListCursor &cursor) const {
            return neighbor_list_.get_neighbor_node(cursor);
        }

        inline nb::Poll<void> poll_send_hello(
            const local::LocalNodeService &local_node_service,
            const link::Address &destination,
            node::Cost link_cost,
            etl::optional<link::MediaPortNumber> port = etl::nullopt
        ) {
            return task_executor_.poll_send_hello(local_node_service, destination, link_cost, port);
        }

        inline nb::Poll<void> poll_send_goodbye(
            const local::LocalNodeService &local_node_service,
            const node::NodeId &destination
        ) {
            return task_executor_.poll_send_goodbye(
                local_node_service, destination, neighbor_list_
            );
        }

        inline void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            util::Time &time
        ) {
            const auto &poll_info = lns.poll_info();
            if (poll_info.is_ready()) {
                task_executor_.execute(fs, nts, poll_info.unwrap(), neighbor_list_, time);
            }
        }
    };
} // namespace net::neighbor
