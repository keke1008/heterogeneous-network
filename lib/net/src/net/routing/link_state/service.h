#pragma once

#include "../neighbor/service.h"
#include "./cache.h"
#include "./frame.h"
#include "./table.h"

namespace net::routing::link_state {
    class SendFrameTask {
        frame::FrameBufferReader reader_;
        link::Address peer_media_;

      public:
        explicit SendFrameTask(frame::FrameBufferReader reader, link::Address peer_media)
            : reader_{etl::move(reader)},
              peer_media_{peer_media} {}

        inline nb::Poll<void> execute(link::LinkService &link_service) {
            return link_service.send_frame(
                frame::ProtocolNumber::Routing, peer_media_, etl::move(reader_)
            );
        }
    };

    class SendFrameBroadCastTask {
        frame::FrameBufferReader reader_;
        etl::vector<NodeId, MAX_PEERS> peers_;
        etl::optional<link::Address> current_peer_media_;

      public:
        explicit SendFrameBroadCastTask(
            frame::FrameBufferReader reader,
            etl::span<const NodeId *> peers
        )
            : reader_{etl::move(reader)} {
            for (auto &peer : peers) {
                peers_.push_back(*peer);
            }
        }

        nb::Poll<void>
        execute(link::LinkService &link_service, neighbor::NeighborService &neighbor_service) {
            if (current_peer_media_.has_value()) {
                POLL_UNWRAP_OR_RETURN(link_service.send_frame(
                    frame::ProtocolNumber::Routing, current_peer_media_.value(), etl::move(reader_)
                ));
                current_peer_media_ = etl::nullopt;
            }

            if (!current_peer_media_.has_value()) {
                while (!current_peer_media_.has_value()) {
                    if (peers_.empty()) {
                        return nb::ready();
                    }

                    auto media = neighbor_service.get_media(peers_.back());
                    if (!media.has_value()) {
                        peers_.pop_back();
                    } else {
                        current_peer_media_ = media.value().front();
                    }
                }
            }

            return nb::pending;
        }
    };

    struct SendTask {
        etl::variant<
            etl::monostate,
            NodeAdditionWriter,
            NodeRemovalWriter,
            etl::pair<NodeId, SyncSolicitationWriter>,
            etl::pair<NodeId, SyncAdvertisementWriter>,
            SendFrameTask,
            SendFrameBroadCastTask>
            task_;

        etl::span<const NodeId *> get_peers(
            neighbor::NeighborService &neighbor_service,
            etl::array<const NodeId *, MAX_PEERS> &buffer
        ) {
            uint8_t len = neighbor_service.get_neighbors(buffer);
            return etl::span(buffer.begin(), len);
        }

      public:
        nb::Poll<void> request_send_node_addition(
            FrameId frame_id,
            const NodeId &added_node_id,
            const NodeId &self_node_id,
            Cost added_node_cost,
            Cost edge_cost
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = NodeAdditionWriter{
                    frame_id, added_node_id, self_node_id, added_node_cost, edge_cost};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        nb::Poll<void> request_send_node_removal(FrameId frame_id, const NodeId &removed_node_id) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = NodeRemovalWriter{frame_id, removed_node_id};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        nb::Poll<void>
        request_send_sync_solicitation(const NodeId &peer_id, const NodeId &self_node_id) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = etl::pair{peer_id, SyncSolicitationWriter{self_node_id}};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        nb::Poll<void> request_send_sync_advertisement(const NodeId &peer_id) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = etl::pair{peer_id, SyncAdvertisementWriter{}};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        nb::Poll<void> request_broadcast(
            neighbor::NeighborService &neighbor_service,
            frame::FrameBufferReader &&reader
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                etl::array<const NodeId *, MAX_PEERS> buffer;
                task_ =
                    SendFrameBroadCastTask{etl::move(reader), get_peers(neighbor_service, buffer)};
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

      private:
        inline void handle_written_broadcast_frame(
            neighbor::NeighborService &neighbor_service,
            frame::FrameBufferReader &&reader
        ) {
            etl::array<const NodeId *, MAX_PEERS> buffer;
            task_ = SendFrameBroadCastTask{etl::move(reader), get_peers(neighbor_service, buffer)};
        }

        inline void handle_written_unicast_frame(
            neighbor::NeighborService &neighbor_service,
            NodeId peer_id,
            frame::FrameBufferReader &&reader
        ) {
            auto media = neighbor_service.get_media(peer_id);
            if (media.has_value()) {
                task_ = SendFrameTask{etl::move(reader), media.value().front()};
            } else {
                task_ = etl::monostate{};
            }
        }

      public:
        template <net::frame::IFrameService FrameService>
        void execute(
            FrameService &frame_service,
            link::LinkService &link_service,
            neighbor::NeighborService &neighbor_service,
            const Table &table
        ) {
            if (etl::holds_alternative<NodeAdditionWriter>(task_)) {
                auto &task = etl::get<NodeAdditionWriter>(task_);
                auto poll_reader = task.execute(frame_service);
                if (poll_reader.is_pending()) {
                    return;
                }
                handle_written_broadcast_frame(etl::move(poll_reader.unwrap()), neighbor_service);
            }

            if (etl::holds_alternative<NodeRemovalWriter>(task_)) {
                auto &task = etl::get<NodeRemovalWriter>(task_);
                auto poll_reader = task.execute(frame_service);
                if (poll_reader.is_pending()) {
                    return;
                }
                handle_written_broadcast_frame(etl::move(poll_reader.unwrap()), neighbor_service);
            }

            if (etl::holds_alternative<etl::pair<NodeId, SyncSolicitationWriter>>(task_)) {
                auto &pair = etl::get<etl::pair<NodeId, SyncSolicitationWriter>>(task_);
                auto &peer_id = pair.first;
                auto &writer = pair.second;
                auto poll_reader = writer.execute(frame_service, table);
                if (poll_reader.is_pending()) {
                    return;
                }
                handle_written_unicast_frame(etl::move(poll_reader.unwrap()), peer_id);
            }

            if (etl::holds_alternative<etl::pair<NodeId, SyncAdvertisementWriter>>(task_)) {
                auto &pair = etl::get<etl::pair<NodeId, SyncAdvertisementWriter>>(task_);
                auto &peer_id = pair.first;
                auto &writer = pair.second;
                auto poll_reader = writer.execute(frame_service, table);
                if (poll_reader.is_pending()) {
                    return;
                }
                handle_written_unicast_frame(etl::move(poll_reader.unwrap()), peer_id);
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &task = etl::get<SendFrameTask>(task_);
                if (task.execute(link_service).is_ready()) {
                    task_ = etl::monostate{};
                }
            }

            if (etl::holds_alternative<SendFrameBroadCastTask>(task_)) {
                auto &task = etl::get<SendFrameBroadCastTask>(task_);
                if (task.execute(link_service, neighbor_service).is_ready()) {
                    task_ = etl::monostate{};
                }
            }
        }
    };

    class ReceiveTask {
        struct RequestSendSyncAdvertisementTask {
            NodeId peer_id;
        };

        struct RequestBroadcastFrame {
            frame::FrameBufferReader reader;
        };

        etl::variant<
            etl::monostate,
            link::ReceivedFrame,
            SyncSolicitationReader,
            SyncAdvertisementReader,
            NodeAdditionReader,
            NodeRemovalReader,
            RequestSendSyncAdvertisementTask,
            RequestBroadcastFrame>
            task_;

      public:
        void execute(
            link::LinkService &link_service,
            neighbor::NeighborService &neighbor_service,
            Table &table,
            SendTask &send_task,
            FrameIdCache &frame_id_cache
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto poll_reader = link_service.receive_frame(frame::ProtocolNumber::Routing);
                if (poll_reader.is_ready()) {
                    task_ = etl::move(poll_reader.unwrap());
                }
            }

            if (etl::holds_alternative<link::ReceivedFrame>(task_)) {
                auto &frame = etl::get<link::ReceivedFrame>(task_);
                if (frame.reader.readable_count() < 1) {
                    return;
                }

                auto frame_type = static_cast<FrameType>(frame.reader.read());
                switch (frame_type) {
                case FrameType::SYNC_SOLICITATION: {
                    task_ = SyncSolicitationReader{etl::move(frame.reader)};
                    break;
                }
                case FrameType::SYNC_ADVERTISEMENT: {
                    task_ = SyncAdvertisementReader{etl::move(frame.reader)};
                    break;
                }
                case FrameType::NODE_ADDITION: {
                    task_ = NodeAdditionReader{etl::move(frame.reader)};
                    break;
                }
                case FrameType::NODE_REMOVAL: {
                    task_ = NodeRemovalReader{etl::move(frame.reader)};
                    break;
                }
                default: {
                    task_ = etl::monostate{};
                    break;
                }
                }
            }

            if (etl::holds_alternative<SyncSolicitationReader>(task_)) {
                auto poll_peer = etl::get<SyncSolicitationReader>(task_).execute();
                if (poll_peer.is_pending()) {
                    return;
                }
                task_ = RequestSendSyncAdvertisementTask{.peer_id = poll_peer.unwrap()};
            }

            if (etl::holds_alternative<SyncAdvertisementReader>(task_)) {
                if (etl::get<SyncAdvertisementReader>(task_).execute(table).is_pending()) {
                    return;
                }
                task_ = etl::monostate{};
            }

            if (etl::holds_alternative<NodeAdditionReader>(task_)) {
                auto poll = etl::get<NodeAdditionReader>(task_).execute(table);
                if (poll.is_pending()) {
                    return;
                }

                auto frame_id = poll.unwrap().first;
                auto &reader = poll.unwrap().second;
                if (frame_id_cache.contains(frame_id)) {
                    task_ = etl::monostate{};
                } else {
                    frame_id_cache.push(frame_id);
                    task_ = RequestBroadcastFrame{.reader = etl::move(reader)};
                }
            }

            if (etl::holds_alternative<NodeRemovalReader>(task_)) {
                auto poll = etl::get<NodeRemovalReader>(task_).execute(table);
                if (poll.is_pending()) {
                    return;
                }

                auto frame_id = poll.unwrap().first;
                auto &reader = poll.unwrap().second;
                if (frame_id_cache.contains(frame_id)) {
                    task_ = etl::monostate{};
                } else {
                    frame_id_cache.push(frame_id);
                    task_ = RequestBroadcastFrame{.reader = etl::move(reader)};
                }
            }

            if (etl::holds_alternative<RequestSendSyncAdvertisementTask>(task_)) {
                auto &task = etl::get<RequestSendSyncAdvertisementTask>(task_);
                if (send_task.request_send_sync_advertisement(task.peer_id).is_ready()) {
                    task_ = etl::monostate{};
                }
            }

            if (etl::holds_alternative<RequestBroadcastFrame>(task_)) {
                auto &task = etl::get<RequestBroadcastFrame>(task_);
                auto poll = send_task.request_broadcast(neighbor_service, etl::move(task.reader));
                if (poll.is_ready()) {
                    task_ = etl::monostate{};
                }
            }
        }
    };

    class LinkStateService {
        SendTask send_task_;
        ReceiveTask receive_frame_task_;
        Table table_;
        FrameIdCache frame_id_cache_;

      public:
        explicit LinkStateService(const NodeId &self_id, Cost self_cost)
            : table_{self_id, self_cost} {}

        inline const NodeId &self_id() const {
            return table_.self_id();
        }

        inline Cost self_cost() const {
            return table_.self_cost();
        }

        etl::optional<NodeId> get_route(const NodeId &node_id) const {
            auto index = table_.get_node(node_id);
            if (!index.has_value()) {
                return etl::nullopt;
            }

            auto route = table_.resolve_route(index.value());
            if (!route.has_value()) {
                return etl::nullopt;
            }

            auto gateway_node = table_.resolve_node(route->gateway);
            if (!gateway_node.has_value()) {
                return etl::nullopt;
            }

            return gateway_node->id;
        }

        void add_node_and_edge(
            const NodeId &node_id,
            Cost node_cost,
            const NodeId &peer_id,
            Cost edge_cost
        ) {
            auto index = table_.get_or_insert_node(node_id, node_cost);
            auto peer_index = table_.get_node(peer_id);
            if (index.has_value() && peer_index.has_value()) {
                table_.insert_edge_if_not_exists(index.value(), peer_index.value(), edge_cost);
            }
        }

        inline void remove_node(const NodeId &node_id) {
            auto index = table_.get_node(node_id);
            if (index.has_value()) {
                table_.remove_node(index.value());
            }
        }

        nb::Poll<void> request_send_node_addition(
            util::Rand &rand,
            const NodeId &added_node_id,
            const NodeId &self_node_id,
            Cost added_node_cost,
            Cost edge_cost
        ) {
            FrameId frame_id = frame_id_cache_.generate(rand);
            return send_task_.request_send_node_addition(
                frame_id, added_node_id, self_node_id, added_node_cost, edge_cost
            );
        }

        nb::Poll<void> request_send_node_removal(util::Rand &rand, const NodeId &removed_node_id) {
            FrameId frame_id = frame_id_cache_.generate(rand);
            return send_task_.request_send_node_removal(frame_id, removed_node_id);
        }

        template <net::frame::IFrameService FrameService>
        inline void execute(
            FrameService &frame_service,
            link::LinkService &link_service,
            neighbor::NeighborService &neighbor_service
        ) {
            receive_frame_task_.execute(
                link_service, neighbor_service, table_, send_task_, frame_id_cache_
            );
            send_task_.execute(frame_service, link_service, neighbor_service);
        }
    };
} // namespace net::routing::link_state
