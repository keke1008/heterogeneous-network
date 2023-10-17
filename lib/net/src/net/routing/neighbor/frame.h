#pragma once

#include "../node.h"
#include "./table.h"
#include <nb/buf.h>
#include <nb/buf/splitter.h>
#include <net/frame/service.h>
#include <net/socket.h>

namespace net::routing::neighbor {
    enum class FrameType : uint8_t {
        HELLO = 0x01,
        GOODBYE = 0x02,
    };

    class FrameWriter {
        FrameType type_;
        NodeId self_node_id_;
        etl::optional<etl::pair<Cost, Cost>> self_node_cost_and_edge_cost_;

      public:
        explicit FrameWriter(const NodeId &self_node_id, Cost self_node_cost, Cost edge_cost)
            : type_{FrameType::HELLO},
              self_node_id_{self_node_id},
              self_node_cost_and_edge_cost_{etl::pair{self_node_cost, edge_cost}} {}

        explicit FrameWriter(const NodeId &self_node_id)
            : type_{FrameType::GOODBYE},
              self_node_id_{self_node_id} {}

        template <net::frame::IFrameService FrameService>
        nb::Poll<frame::FrameBufferReader> execute(FrameService &frame_service) {
            uint8_t length = 1 + NodeId::MAX_LENGTH;
            if (self_node_cost_and_edge_cost_.has_value()) {
                length += Cost::LENGTH * 2;
            }

            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));
            writer.write(static_cast<uint8_t>(type_), self_node_id_);
            if (self_node_cost_and_edge_cost_.has_value()) {
                writer.write(self_node_cost_and_edge_cost_->first);
                writer.write(self_node_cost_and_edge_cost_->second);
            }

            writer.shrink_frame_length_to_fit();
            return writer.make_initial_reader();
        }
    };

    struct HelloFrame {
        NodeId peer_id;
        link::Address peer_media;
        Cost peer_cost;
        Cost edge_cost;
    };

    struct GoodbyeFrame {
        NodeId peer_id;
    };

    class FrameReader {
        link::ReceivedFrame frame_;

      public:
        explicit FrameReader(link::ReceivedFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<etl::variant<HelloFrame, GoodbyeFrame>> execute() {
            if (!frame_.reader.is_buffer_filled()) {
                return nb::pending;
            }

            auto type = static_cast<FrameType>(frame_.reader.read());
            auto peer = frame_.reader.read<NodeIdParser>();
            if (type == FrameType::HELLO) {
                return etl::variant<HelloFrame, GoodbyeFrame>{HelloFrame{
                    .peer_id = peer,
                    .peer_media = frame_.peer,
                    .peer_cost = frame_.reader.read<CostParser>(),
                    .edge_cost = frame_.reader.read<CostParser>(),
                }};
            } else {
                return etl::variant<HelloFrame, GoodbyeFrame>{
                    GoodbyeFrame{.peer_id = peer},
                };
            }
        }
    };
} // namespace net::routing::neighbor
