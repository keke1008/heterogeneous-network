#pragma once

#include "./table.h"

namespace net::routing::link_state {
    class FrameId {
        uint8_t id_;

      public:
        FrameId() = delete;
        FrameId(const FrameId &) = default;
        FrameId(FrameId &&) = default;
        FrameId &operator=(const FrameId &) = default;
        FrameId &operator=(FrameId &&) = default;

        inline explicit FrameId(uint8_t id) : id_{id} {}

        inline bool operator==(const FrameId &other) const {
            return id_ == other.id_;
        }

        inline bool operator!=(const FrameId &other) const {
            return id_ != other.id_;
        }

        constexpr inline uint8_t serialized_length() const {
            return 1;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(id_);
        }
    };

    class FrameIdParser {
      public:
        inline FrameId parse(nb::buf::BufferSplitter &splitter) const {
            return FrameId{splitter.split_1byte()};
        }
    };

    static constexpr uint8_t FRAME_TYPE_LENGTH = 1;

    enum class FrameType : uint8_t {
        // フレームフォーマット
        // 1. 送信元ノードID
        SYNC_SOLICITATION = 0x01,

        // フレームフォーマット
        // 1. 隣接ノードID
        // 2. 隣接ノードコスト
        // 3. 隣接ノードの隣接ノードのID
        // 4. 隣接ノードの隣接ノードコスト
        // 5. 3, 4を隣接ノードの隣接の数だけ繰り返す
        // 6. 1, 2, 3, 4, 5を隣接ノードの数だけ繰り返す
        SYNC_ADVERTISEMENT = 0x02,

        // フレームフォーマット
        //
        // 1. フレームID
        // 2. 追加されたノードのID
        // 3. 追加されたノードの隣接ノードのID
        // 4. 追加されたノードのコスト
        // 5. 2と3のノード間のコスト
        NODE_ADDITION = 0x03,

        // フレームフォーマット
        //
        // 1. フレームID
        // 2. 削除されたノードのID
        NODE_REMOVAL = 0x04,
    };

    class NodeAdditionWriter {
        FrameId frame_id_;
        NodeId added_node_id_;
        NodeId self_node_id_;
        Cost added_node_cost_;
        Cost edge_cost_;

      public:
        explicit NodeAdditionWriter(
            FrameId frame_id,
            const NodeId &added_node_id,
            const NodeId &self_node_id,
            Cost added_node_cost,
            Cost edge_cost
        )
            : frame_id_{frame_id},
              added_node_id_{added_node_id},
              self_node_id_{self_node_id},
              added_node_cost_{added_node_cost},
              edge_cost_{edge_cost} {}

        template <net::frame::IFrameService FrameService>
        nb::Poll<frame::FrameBufferReader> execute(FrameService &frame_service) {
            uint8_t length = FRAME_TYPE_LENGTH +       // フレームタイプ
                frame_id_.serialized_length() +        // フレームID
                added_node_id_.serialized_length() +   // 追加されたノードID
                self_node_id_.serialized_length() +    // 自ノードID
                added_node_cost_.serialized_length() + // 追加されたノードコスト
                edge_cost_.serialized_length(); // 追加されたノードと自ノード間のコスト
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));

            writer.write(static_cast<uint8_t>(FrameType::NODE_ADDITION));
            writer.write(added_node_id_, self_node_id_, added_node_cost_, edge_cost_);
            return writer.make_initial_reader();
        }
    };

    class NodeAdditionReader {
        frame::FrameBufferReader reader_;

      public:
        explicit NodeAdditionReader(frame::FrameBufferReader reader) : reader_{etl::move(reader)} {}

        nb::Poll<etl::pair<FrameId, frame::FrameBufferReader>> execute(Table &table) {
            if (!reader_.is_buffer_filled()) {
                return nb::pending;
            }

            auto frame_id = reader_.read<FrameIdParser>();
            auto node_id = reader_.read<NodeIdParser>();
            auto peer_id = reader_.read<NodeIdParser>();
            auto node_cost = reader_.read<CostParser>();
            auto edge_cost = reader_.read<CostParser>();

            auto node_index = table.get_or_insert_node(node_id, node_cost);
            if (!node_index.has_value()) {
                return etl::pair{frame_id, etl::move(reader_)};
            }

            auto peer_index = table.get_node(peer_id);
            if (!peer_index.has_value()) {
                return etl::pair{frame_id, etl::move(reader_)};
            }

            table.insert_edge_if_not_exists(node_index.value(), peer_index.value(), edge_cost);
            return etl::pair{frame_id, reader_.make_initial_clone()};
        }
    };

    class NodeRemovalWriter {
        FrameId frame_id_;
        NodeId removed_node_id_;

      public:
        explicit NodeRemovalWriter(FrameId frame_id, const NodeId &removed_node_id)
            : frame_id_{frame_id},
              removed_node_id_{removed_node_id} {}

        template <net::frame::IFrameService FrameService>
        nb::Poll<frame::FrameBufferReader> execute(FrameService &frame_service) {
            uint8_t length = FRAME_TYPE_LENGTH + removed_node_id_.serialized_length();
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));

            writer.write(static_cast<uint8_t>(FrameType::NODE_REMOVAL));
            writer.write(removed_node_id_);
            return writer.make_initial_reader();
        }
    };

    class NodeRemovalReader {
        frame::FrameBufferReader reader_;

      public:
        explicit NodeRemovalReader(frame::FrameBufferReader reader) : reader_{etl::move(reader)} {}

        nb::Poll<etl::pair<FrameId, frame::FrameBufferReader>> execute(Table &table) {
            if (!reader_.is_buffer_filled()) {
                return nb::pending;
            }

            auto frame_id = reader_.read<FrameIdParser>();
            auto node_id = reader_.read<NodeIdParser>();
            auto node_index = table.get_node(node_id);
            if (node_index.has_value()) {
                table.remove_node(node_index.value());
            }

            return etl::pair{frame_id, reader_.make_initial_clone()};
        }
    };

    class SyncSolicitationWriter {
        NodeId self_node_id_;

      public:
        explicit SyncSolicitationWriter(const NodeId &self_node_id) : self_node_id_{self_node_id} {}

        template <net::frame::IFrameService FrameService>
        nb::Poll<frame::FrameBufferReader>
        execute(FrameService &frame_service, const Table &table) {
            uint8_t length = FRAME_TYPE_LENGTH + self_node_id_.serialized_length();
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));
            writer.write(static_cast<uint8_t>(FrameType::SYNC_SOLICITATION));
            writer.write(self_node_id_);
            return writer.make_initial_reader();
        }
    };

    class SyncSolicitationReader {
        frame::FrameBufferReader reader_;

      public:
        explicit SyncSolicitationReader(frame::FrameBufferReader reader)
            : reader_{etl::move(reader)} {}

        nb::Poll<NodeId> execute() {
            if (!reader_.is_buffer_filled()) {
                return nb::pending;
            }

            auto node_id = reader_.read<NodeIdParser>();
            return node_id;
        }
    };

    class SyncAdvertisementWriter {
      public:
        template <net::frame::IFrameService FrameService>
        nb::Poll<frame::FrameBufferReader>
        execute(FrameService &frame_service, const Table &table) {
            auto writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_max_length_frame_writer());

            writer.write(static_cast<uint8_t>(FrameType::SYNC_ADVERTISEMENT));

            etl::array<uint8_t, MAX_NODES> table_index_to_frame_index;
            uint8_t frame_index = 0;

            for (NodeIndex node_index = 0; node_index < MAX_NODES; node_index++) {
                auto &node = table.resolve_node(node_index);
                if (!node.has_value()) {
                    continue;
                }

                uint8_t max_length = node->id.serialized_length() +           // ノードID
                    node->cost.serialized_length() +                          // ノードコスト
                    1 +                                                       // peerノード数
                    node->peers.size() * (NodeId::MAX_LENGTH + Cost::LENGTH); // peerノード
                if (writer.writable_count() < max_length) {
                    break;
                }

                table_index_to_frame_index[node_index] = frame_index++;

                writer.write(node->id, node->cost);
                writer.write(static_cast<uint8_t>(node->peers.size()));
                auto &peers = node->peers;
                for (auto &peer : peers) {
                    if (peer.node_index < node_index) {
                        writer.write(table_index_to_frame_index[peer.node_index], peer.cost);
                    }
                }
            }

            writer.shrink_frame_length_to_fit();
            return writer.make_initial_reader();
        }
    };

    class SyncAdvertisementReader {
        frame::FrameBufferReader reader_;

      public:
        explicit SyncAdvertisementReader(frame::FrameBufferReader reader)
            : reader_{etl::move(reader)} {}

        nb::Poll<void> execute(Table &table) {
            if (!reader_.is_buffer_filled()) {
                return nb::pending;
            }

            etl::array<NodeIndex, MAX_NODES> frame_index_to_table_index;
            uint8_t node_count = 0;

            while (reader_.readable_count() > 0) {
                auto node_id = reader_.read<NodeIdParser>();
                auto cost = reader_.read<CostParser>();
                auto opt_index = table.get_or_insert_node(node_id, cost);
                if (!opt_index.has_value()) {
                    return nb::ready();
                }

                auto node_index = opt_index.value();
                frame_index_to_table_index[node_count++] = node_index;

                auto peer_count = reader_.read();
                for (uint8_t i = 0; i < peer_count; i++) {
                    auto peer_frame_index = reader_.read();
                    auto peer_cost = reader_.read<CostParser>();
                    auto peer_table_index = frame_index_to_table_index[peer_frame_index];
                    table.insert_edge_if_not_exists(node_index, peer_table_index, peer_cost);
                }
            }

            return nb::ready();
        }
    };
} // namespace net::routing::link_state
