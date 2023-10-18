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
        struct SendHelloAckTask {
            link::Address destination;
            Cost edge_cost;
        };

        etl::variant<etl::monostate, FrameReader, SendHelloAckTask> task_;

      public:
        Event execute(
            link::LinkService &link_service,
            NeighborList &neighbor_list,
            SendFrameTask &send_task,
            const NodeId &self_node_id,
            Cost self_node_cost
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto poll_frame =
                    link_service.receive_frame(frame::ProtocolNumber::RoutingNeighbor);
                if (poll_frame.is_pending()) {
                    return etl::monostate{};
                }
                task_ = FrameReader(etl::move(poll_frame.unwrap()));
            }

            if (etl::holds_alternative<FrameReader>(task_)) {
                auto poll_frame = etl::get<FrameReader>(task_).execute();
                if (poll_frame.is_pending()) {
                    return etl::monostate{};
                }
                task_ = etl::monostate{};
                if (etl::holds_alternative<HelloFrame>(poll_frame.unwrap())) {
                    auto &frame = etl::get<HelloFrame>(poll_frame.unwrap());
                    if (!frame.is_ack) {
                        task_.emplace<SendHelloAckTask>(frame.peer_media, frame.edge_cost);
                    }
                    auto result = neighbor_list.add_neighbor_node(frame.peer_id, frame.peer_media);
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
                    if (result == RemoveNodeResult::Disconnected) {
                        return Event{NodeDisconnectedEvent{.id = frame.peer_id}};
                    }
                }
            }

            if (etl::holds_alternative<SendHelloAckTask>(task_)) {
                auto &task = etl::get<SendHelloAckTask>(task_);
                auto poll_task = send_task.request_send_frame(
                    task.destination,
                    FrameWriter::HelloAck(self_node_id, self_node_cost, task.edge_cost)
                );
                if (poll_task.is_ready()) {
                    task_ = etl::monostate{};
                } else {
                    return etl::monostate{};
                }
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

        inline nb::Poll<void> request_hello(
            const link::Address &destination,
            const NodeId &self_node_id,
            Cost self_node_cost,
            Cost edge_cost
        ) {
            return send_task_.request_send_frame(
                destination, FrameWriter::Hello(self_node_id, self_node_cost, edge_cost)
            );
        }

        inline nb::Poll<void>
        request_goodbye(const NodeId destination, const NodeId &self_node_id) {
            auto media_list = neighbor_list_.get_media(self_node_id);
            if (media_list.has_value()) {
                neighbor_list_.remove_neighbor_node(destination);
                auto &media = media_list.value().front();
                return send_task_.request_send_frame(media, FrameWriter::Goodbye(self_node_id));
            } else {
                return nb::ready();
            }
        }

        template <frame::IFrameService FrameService>
        Event execute(
            FrameService &frame_service,
            link::LinkService &link_service,
            const NodeId &self_node_id,
            Cost self_node_cost
        ) {
            send_task_.execute(frame_service);
            return receive_task_.execute(
                link_service, neighbor_list_, send_task_, self_node_id, self_node_cost
            );
        }
    };
} // namespace net::routing::neighbor
