#pragma once

#include "../node.h"
#include "./table.h"
#include <nb/buf.h>
#include <nb/buf/splitter.h>
#include <net/frame/service.h>
#include <net/socket.h>

namespace net::routing::neighbor {
    enum class FrameType : uint8_t {
        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者のノードID
        // 3. 送信者のノードコスト
        // 4. リンクコスト
        HELLO = 0x01,

        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者のノードID
        // 3. 送信者のノードコスト
        // 4. リンクコスト
        HELLO_ACK = 0x02,

        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者のノードID
        GOODBYE = 0x03,
    };

    class FrameWriter {
        FrameType type_;
        NodeId self_node_id_;
        etl::optional<Cost> link_cost_;

        explicit FrameWriter(FrameType type, const NodeId &self_node_id, Cost link_cost)
            : type_{type},
              self_node_id_{self_node_id},
              link_cost_{link_cost} {}

        explicit FrameWriter(const NodeId &self_node_id)
            : type_{FrameType::GOODBYE},
              self_node_id_{self_node_id} {}

      public:
        inline static FrameWriter Hello(const NodeId &self_node_id, Cost edge_cost) {
            return FrameWriter{FrameType::HELLO, self_node_id, edge_cost};
        }

        inline static FrameWriter HelloAck(const NodeId &self_node_id, Cost edge_cost) {
            return FrameWriter{FrameType::HELLO_ACK, self_node_id, edge_cost};
        }

        inline static FrameWriter Goodbye(const NodeId &self_node_id) {
            return FrameWriter{self_node_id};
        }

        nb::Poll<frame::FrameBufferReader> execute(frame::FrameService &frame_service) {
            uint8_t length = 1 + NodeId::MAX_LENGTH;
            if (link_cost_.has_value()) {
                length += Cost::LENGTH * 2;
            }

            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));
            writer.write(
                nb::buf::FormatBinary<uint8_t>(static_cast<uint8_t>(type_)), self_node_id_
            );
            if (link_cost_.has_value()) {
                writer.write(*link_cost_);
            }

            writer.shrink_frame_length_to_fit();
            return writer.make_initial_reader();
        }
    };

    inline void write_frame(
        frame::FrameBufferWriter &writer,
        FrameType type,
        const NodeId &self_node_id,
        etl::optional<Cost> link_cost
    ) {}

    struct HelloFrame {
        bool is_ack;
        NodeId sender_id;
        Cost node_cost;
        Cost link_cost;

        inline uint8_t serialized_length() const {
            return 1 + sender_id.serialized_length() + node_cost.serialized_length() +
                link_cost.serialized_length();
        }

        inline FrameType frame_type() const {
            return is_ack ? FrameType::HELLO_ACK : FrameType::HELLO;
        }

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            uint8_t type = static_cast<uint8_t>(frame_type());
            builder.append(nb::buf::FormatBinary<uint8_t>(type));
            builder.append(sender_id);
            builder.append(node_cost);
            builder.append(link_cost);
        }

        static HelloFrame parse(FrameType type, nb::buf::BufferSplitter &splitter) {
            return HelloFrame{
                .is_ack = type == FrameType::HELLO_ACK,
                .sender_id = splitter.parse<NodeIdParser>(),
                .node_cost = splitter.parse<CostParser>(),
                .link_cost = splitter.parse<CostParser>(),
            };
        };
    };

    struct GoodbyeFrame {
        NodeId sender_id;

        inline uint8_t serialized_length() const {
            return 1 + sender_id.serialized_length();
        }

        inline constexpr FrameType frame_type() const {
            return FrameType::GOODBYE;
        }

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            uint8_t type = static_cast<uint8_t>(frame_type());
            builder.append(nb::buf::FormatBinary<uint8_t>(type));
            builder.append(sender_id);
        }

        static inline GoodbyeFrame parse(nb::buf::BufferSplitter &splitter) {
            auto sender = splitter.template parse<NodeIdParser>();
            return GoodbyeFrame{.sender_id = sender};
        };
    };

    using NeighborFrame = etl::variant<HelloFrame, GoodbyeFrame>;

    etl::optional<NeighborFrame> parse_frame(etl::span<const uint8_t> buffer);
} // namespace net::routing::neighbor
