#pragma once

#include "../constants.h"
#include "./frame_type.h"
#include <net/frame.h>
#include <net/routing.h>

namespace net::observer {
    class NodeSyncFrameWriter {
        using NeighborCountSerializer = nb::ser::Bin<uint8_t>;

        uint8_t
        calculate_length(const local::LocalNodeInfo &info, const neighbor::NeighborService &ns) {
            uint8_t length = 0;

            length += AsyncFrameTypeSerializer::serialized_length(FrameType::NodeSync);

            length += node::AsyncSourceSerializer::serialized_length(info.source);
            length += node::AsyncCostSerializer::serialized_length(info.cost);

            length += NeighborCountSerializer::serialized_length(ns.get_neighbor_count());
            ns.for_each_neighbor_node([&](const neighbor::NeighborNode &neighbor) {
                length += node::AsyncNodeIdSerializer::serialized_length(neighbor.id());
                length += node::AsyncCostSerializer::serialized_length(neighbor.link_cost());
            });

            return length;
        }

        frame::FrameBufferReader write_frame(
            const local::LocalNodeInfo &info,
            const neighbor::NeighborService &ns,
            frame::FrameBufferWriter &&writer
        ) {
            writer.serialize_all_at_once(AsyncFrameTypeSerializer(FrameType::NodeSync));

            writer.serialize_all_at_once(node::AsyncSourceSerializer(info.source));
            writer.serialize_all_at_once(node::AsyncCostSerializer(info.cost));

            writer.serialize_all_at_once(NeighborCountSerializer(ns.get_neighbor_count()));
            ns.for_each_neighbor_node([&](const neighbor::NeighborNode &neighbor) {
                writer.serialize_all_at_once(node::AsyncNodeIdSerializer(neighbor.id()));
                writer.serialize_all_at_once(node::AsyncCostSerializer(neighbor.link_cost()));
            });

            FASSERT(writer.is_all_written());
            return writer.create_reader();
        }

      public:
        nb::Poll<frame::FrameBufferReader> execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService &ns,
            routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> &socket,
            const node::Destination &destination,
            util::Rand &rand

        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            uint8_t length = calculate_length(info, ns);

            auto &&writer = POLL_MOVE_UNWRAP_OR_RETURN(
                socket.poll_frame_writer(fs, lns, rand, destination, length)
            );
            return write_frame(info, ns, etl::move(writer));
        }
    };
} // namespace net::observer
