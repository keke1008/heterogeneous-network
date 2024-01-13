#pragma once

#include <net/frame.h>
#include <net/node.h>

namespace net::neighbor::service {
    struct NeighborControlFlags {

        // フレームがノードの生存確認のためのものであることを示す．
        // このフラグが立っている場合，フレーム中のlink_costは無視すべきである．
        uint8_t keep_alive : 1;

        static inline NeighborControlFlags EMPTY() {
            return NeighborControlFlags{
                .keep_alive = 0,
            };
        }

        static inline NeighborControlFlags KEEP_ALIVE() {
            return NeighborControlFlags{
                .keep_alive = 1,
            };
        }

        static inline bool is_valid_representation(uint8_t byte) {
            return (byte & 0b11111110) == 0;
        }

        static inline NeighborControlFlags from_byte(uint8_t byte) {
            return NeighborControlFlags{
                .keep_alive = (byte & 0b00000001) != 0,
            };
        }

        inline constexpr uint8_t to_byte() const {
            return (keep_alive << 0);
        }

        inline constexpr bool should_reply_immediately() const {
            return !keep_alive;
        }
    };

    class AsyncNeighborControlFlagsDeserializer {
        nb::de::Bin<uint8_t> flags_;

      public:
        inline NeighborControlFlags result() const {
            return NeighborControlFlags::from_byte(flags_.result());
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(flags_.deserialize(r));
            return NeighborControlFlags::is_valid_representation(flags_.result())
                ? nb::DeserializeResult::Ok
                : nb::DeserializeResult::Invalid;
        }
    };

    class AsyncNeighborControlFlagsSerializer {
        nb::ser::Bin<uint8_t> flags_;

      public:
        explicit inline AsyncNeighborControlFlagsSerializer(const NeighborControlFlags &flags)
            : flags_{flags.to_byte()} {}

        inline constexpr uint8_t serialized_length() const {
            return flags_.serialized_length();
        }

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            return flags_.serialize(w);
        }
    };

    struct NeighborControlFrame {
        NeighborControlFlags flags;
        node::NodeId source_node_id;
        node::Cost link_cost;
    };

    class AsyncNeighborControlFrameDeserializer {
        AsyncNeighborControlFlagsDeserializer flags_;
        node::AsyncNodeIdDeserializer source_node_id_;
        node::AsyncCostDeserializer link_cost_;

      public:
        inline NeighborControlFrame result() const {
            return NeighborControlFrame{
                .flags = flags_.result(),
                .source_node_id = source_node_id_.result(),
                .link_cost = link_cost_.result(),
            };
        }

        template <nb::AsyncReadable R>
        nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(flags_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_node_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(link_cost_.deserialize(r));
            return nb::DeserializeResult::Ok;
        }
    };

    class AsyncNeighborControlFrameSerializer {
        AsyncNeighborControlFlagsSerializer flags_;
        node::AsyncNodeIdSerializer source_node_id_;
        node::AsyncCostSerializer link_cost_;

      public:
        explicit AsyncNeighborControlFrameSerializer(const NeighborControlFrame &frame)
            : flags_{frame.flags},
              source_node_id_{frame.source_node_id},
              link_cost_{frame.link_cost} {}

        inline uint8_t serialized_length() const {
            return flags_.serialized_length() + source_node_id_.serialized_length() +
                link_cost_.serialized_length();
        }

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(flags_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_node_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(link_cost_.serialize(w));
            return nb::SerializeResult::Ok;
        }
    };
} // namespace net::neighbor::service
