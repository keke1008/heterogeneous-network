#pragma once

#include "../node.h"
#include "./frame.h"
#include "./table.h"
#include <etl/variant.h>
#include <nb/buf/splitter.h>
#include <nb/poll.h>
#include <net/frame/service.h>
#include <net/link.h>

namespace net::routing::neighbor {
    struct NodeConnectedEvent {
        NodeId id;
        Cost node_cost;
        Cost edge_cost;
    };

    struct NodeDisconnectedEvent {
        NodeId id;
    };

    using Event = etl::variant<etl::monostate, NodeConnectedEvent, NodeDisconnectedEvent>;

    struct WrittenFrame {
        link::Address destination;
        frame::FrameBufferReader reader;
    };

    class WriteFrameTask {
        link::Address destination_;
        FrameWriter writer_;

      public:
        WriteFrameTask(const link::Address &destination, FrameWriter &&writer)
            : destination_{destination},
              writer_{etl::move(writer)} {}

        template <frame::IFrameService FrameService>
        nb::Poll<WrittenFrame> execute(FrameService &frame_service) {
            return WrittenFrame{
                .destination = destination_,
                .reader = POLL_MOVE_UNWRAP_OR_RETURN(writer_.execute(frame_service)),
            };
        }
    };

    class SendFrameTask {
        etl::variant<etl::monostate, WriteFrameTask, WrittenFrame> task_;

      public:
        nb::Poll<void> request_send_frame(const link::Address &destination, FrameWriter &&writer) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = WriteFrameTask{destination, etl::move(writer)};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        template <frame::IFrameService FrameService>
        void execute(FrameService &frame_service, link::LinkService &link_service) {
            if (etl::holds_alternative<WriteFrameTask>(task_)) {
                auto &task = etl::get<WriteFrameTask>(task_);
                auto poll_task = task.execute(frame_service);
                if (poll_task.is_ready()) {
                    task_ = etl::move(poll_task.unwrap());
                } else {
                    return;
                }
            }

            if (etl::holds_alternative<WrittenFrame>(task_)) {
                auto &frame = etl::get<WrittenFrame>(task_);
                auto poll = link_service.send_frame(
                    frame::ProtocolNumber::RoutingNeighbor, frame.destination,
                    etl::move(frame.reader)
                );
                if (poll.is_ready()) {
                    task_ = etl::monostate{};
                } else {
                    return;
                }
            }
        }
    };

    class ReceiveFrameTask {
        etl::optional<FrameReader> reader_;

      public:
        Event execute(link::LinkService &link_service, NeighborList &neighbor_list) {
            if (reader_.has_value()) {
                auto poll_frame = reader_->execute();
                if (poll_frame.is_pending()) {
                    return etl::monostate{};
                }
                if (etl::holds_alternative<HelloFrame>(poll_frame.unwrap())) {
                    auto &frame = etl::get<HelloFrame>(poll_frame.unwrap());
                    auto result = neighbor_list.add_neighbor_node(frame.peer_id, frame.peer_media);
                    reader_.reset();
                    if (result == AddNodeResult::Connected) {
                        return Event{NodeConnectedEvent{
                            .id = frame.peer_id,
                            .node_cost = frame.peer_cost,
                            .edge_cost = frame.edge_cost,
                        }};
                    }
                } else if (etl::holds_alternative<GoodbyeFrame>(poll_frame.unwrap())) {
                    auto &frame = etl::get<GoodbyeFrame>(poll_frame.unwrap());
                    auto result = neighbor_list.remove_neighbor_node(frame.peer_id);
                    reader_.reset();
                    if (result == RemoveNodeResult::Disconnected) {
                        return Event{NodeDisconnectedEvent{.id = frame.peer_id}};
                    }
                }
            }

            if (!reader_.has_value()) {
                auto poll_frame =
                    link_service.receive_frame(frame::ProtocolNumber::RoutingNeighbor);
                if (poll_frame.is_pending()) {
                    return etl::monostate{};
                }
                reader_ = FrameReader(etl::move(poll_frame.unwrap()));
            }

            return etl::monostate{};
        }
    };

    class NeighborService {
        NeighborList neighbor_list_;
        SendFrameTask send_task_;
        ReceiveFrameTask receive_task_;

      public:
        etl::optional<etl::span<const link::Address>> get_media(const NodeId &node_id) const {
            return neighbor_list_.get_media(node_id);
        }

        uint8_t get_neighbors(etl::span<const NodeId *> neighbors) const {
            return neighbor_list_.get_neighbors(neighbors);
        }

        nb::Poll<void> request_send_hello(
            const link::Address &destination,
            const NodeId &self_node_id,
            Cost self_node_cost,
            Cost edge_cost
        ) {
            FrameWriter writer{self_node_id, self_node_cost, edge_cost};
            return send_task_.request_send_frame(destination, etl::move(writer));
        }

        nb::Poll<void>
        request_send_goodbye(const link::Address &destination, const NodeId &self_node_id) {
            FrameWriter writer{self_node_id};
            return send_task_.request_send_frame(destination, etl::move(writer));
        }

        template <frame::IFrameService FrameService>
        Event execute(FrameService &frame_service, link::LinkService &link_service) {
            send_task_.execute(frame_service);
            return receive_task_.execute(link_service, neighbor_list_);
        }
    };
} // namespace net::routing::neighbor
