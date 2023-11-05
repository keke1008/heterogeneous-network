#pragma once

#include "../node.h"
#include <net/frame.h>
#include <stdint.h>

namespace net::routing::reactive {
    enum class RouteDiscoveryFrameType : uint8_t {
        REQUEST = 0x01,
        REPLY = 0x02,
    };

    struct RouteDiscoveryFrame {
        RouteDiscoveryFrameType type;
        frame::FrameId frame_id;
        Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        NodeId source_id; // 探索を開始したノード
        NodeId target_id; // 探索の対象となるノード
        NodeId sender_id; // このフレームを送信したノード

        static RouteDiscoveryFrame deserialize(etl::span<const uint8_t> bytes) {
            nb::buf::BufferSplitter splitter{bytes};
            return RouteDiscoveryFrame{
                .type = static_cast<RouteDiscoveryFrameType>(splitter.split_1byte()),
                .frame_id = splitter.parse<frame::FrameIdDeserializer>(),
                .total_cost = splitter.parse<CostParser>(),
                .source_id = splitter.parse<NodeIdParser>(),
                .target_id = splitter.parse<NodeIdParser>(),
                .sender_id = splitter.parse<NodeIdParser>(),
            };
        }

        inline uint8_t serialized_length() const {
            return 1 + frame_id.serialized_length() + total_cost.serialized_length() +
                source_id.serialized_length() + target_id.serialized_length() +
                sender_id.serialized_length();
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(static_cast<uint8_t>(type));
            frame_id.write_to_builder(builder);
            total_cost.write_to_builder(builder);
            source_id.write_to_builder(builder);
            target_id.write_to_builder(builder);
            sender_id.write_to_builder(builder);
        }
    };
} // namespace net::routing::reactive
