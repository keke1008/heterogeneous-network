#pragma once

#include "./service/hello.h"
#include "./service/table.h"
#include "./service/task.h"
#include <nb/poll.h>
#include <net/frame.h>
#include <net/link.h>
#include <net/local.h>
#include <net/notification.h>

namespace net::neighbor {
    class NeighborService {
        NeighborList neighbor_list_;
        service::TaskExecutor<MAX_NEIGNBOR_FRAME_DELAY_POOL_SIZE> task_executor_;
        service::SendHelloWorker send_hello_worker_;

      public:
        explicit NeighborService(link::LinkService &link_service, util::Time &time)
            : neighbor_list_{time},
              task_executor_{link_service.open(frame::ProtocolNumber::RoutingNeighbor)},
              send_hello_worker_{time} {}

        inline etl::optional<node::Cost> get_link_cost(const node::NodeId &neighbor_id) const {
            return neighbor_list_.get_link_cost(neighbor_id);
        }

        inline bool has_neighbor(const node::NodeId &neighbor_id) const {
            return neighbor_list_.has_neighbor_node(neighbor_id);
        }

        inline etl::optional<etl::span<const NeighborNodeAddress>>
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

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        resolve_neighbor_node_from_address(const link::Address &address) const {
            return neighbor_list_.resolve_neighbor_node_from_address(address);
        }

        inline void on_frame_received(const node::NodeId &source_id, util::Time &time) {
            neighbor_list_.delay_expiration(source_id, time);
        }

        inline void on_frame_sent(const node::NodeId &destination_id, util::Time &time) {
            neighbor_list_.delay_hello_interval(destination_id, time);
        }

        inline void on_frame_sent(link::AddressTypeSet types, util::Time &time) {
            neighbor_list_.delay_hello_interval(types, time);
        }

        inline nb::Poll<void> poll_send_hello(
            link::LinkService &ls,
            const local::LocalNodeService &lns,
            const link::Address &destination,
            node::Cost link_cost,
            etl::optional<link::MediaPortNumber> port
        ) {

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            auto mask = port.has_value() ? link::MediaPortMask::from_port_number(*port)
                                         : link::MediaPortMask::unspecified();
            return task_executor_.poll_send_initial_hello(info, mask, destination, link_cost);
        }

        inline void execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            util::Time &time
        ) {
            neighbor_list_.execute(nts, time);

            const auto &poll_info = lns.poll_info();
            if (poll_info.is_ready()) {
                const auto &info = poll_info.unwrap();
                task_executor_.execute(fs, nts, info, neighbor_list_, time);
                send_hello_worker_.execute(ms, neighbor_list_, lns, task_executor_, time);
            }
        }
    };
} // namespace net::neighbor
