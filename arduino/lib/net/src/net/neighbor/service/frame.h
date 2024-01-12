#pragma once

#include "../constants.h"
#include <net/frame.h>
#include <net/node.h>

namespace net::neighbor::service {
    struct NeighborControlFlags {

        // フレームがノードの生存確認のためのものであることを示す．
        // このフラグが立っている場合，フレーム中のlink_costは無視すべきである．
        uint8_t keep_alive : 1;

        // フレームがノードのメディアアドレスの問い合わせのためのものであることを示す．
        uint8_t request_media_addresses : 1;

        static inline NeighborControlFlags EMPTY() {
            return NeighborControlFlags{
                .keep_alive = 0,
                .request_media_addresses = 0,
            };
        }

        static inline NeighborControlFlags KEEP_ALIVE() {
            return NeighborControlFlags{
                .keep_alive = 1,
                .request_media_addresses = 0,
            };
        }

        static inline NeighborControlFlags REQUEST_MEDIA_ADDRESSES() {
            return NeighborControlFlags{
                .keep_alive = 0,
                .request_media_addresses = 1,
            };
        }

        static inline NeighborControlFlags media_addresses() {
            return NeighborControlFlags{
                .keep_alive = 0,
                .request_media_addresses = 1,
            };
        }

        static inline bool is_valid_representation(uint8_t byte) {
            return (byte & 0b11111000) == 0;
        }

        static inline NeighborControlFlags from_byte(uint8_t byte) {
            return NeighborControlFlags{
                .keep_alive /*        */ = (byte & 0b00000001) != 0,
                .request_media_addresses = (byte & 0b00000010) != 0
            };
        }

        inline constexpr uint8_t to_byte() const {
            return (keep_alive << 0) | (request_media_addresses << 1);
        }

        inline constexpr bool should_reply_immediately() const {
            return !keep_alive || request_media_addresses;
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
        node::Source source;
        node::Cost source_cost;
        node::Cost link_cost;
        etl::vector<link::Address, MAX_MEDIA_PER_NODE> media_addresses;
    };

    class AsyncNeighborControlFrameDeserializer {
        AsyncNeighborControlFlagsDeserializer flags_;
        node::AsyncSourceDeserializer source_;
        node::AsyncCostDeserializer source_cost_;
        node::AsyncCostDeserializer link_cost_;
        nb::de::Vec<link::AsyncAddressDeserializer, MAX_MEDIA_PER_NODE> media_addresses_;

      public:
        inline NeighborControlFrame result() const {
            return NeighborControlFrame{
                .flags = flags_.result(),
                .source = source_.result(),
                .source_cost = source_cost_.result(),
                .link_cost = link_cost_.result(),
                .media_addresses = media_addresses_.result(),
            };
        }

        template <nb::AsyncReadable R>
        nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(flags_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_cost_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(link_cost_.deserialize(r));
            return media_addresses_.deserialize(r);
        }
    };

    class AsyncNeighborControlFrameSerializer {
        AsyncNeighborControlFlagsSerializer flags_;
        node::AsyncSourceSerializer source_;
        node::AsyncCostSerializer source_cost_;
        node::AsyncCostSerializer link_cost_;
        nb::ser::Vec<link::AsyncAddressSerializer, MAX_MEDIA_PER_NODE> media_addresses_;

      public:
        explicit AsyncNeighborControlFrameSerializer(const NeighborControlFrame &frame)
            : flags_{frame.flags},
              source_{frame.source},
              source_cost_{frame.source_cost},
              link_cost_{frame.link_cost},
              media_addresses_{frame.media_addresses} {}

        inline uint8_t serialized_length() const {
            return flags_.serialized_length() + source_.serialized_length() +
                source_cost_.serialized_length() + link_cost_.serialized_length() +
                media_addresses_.serialized_length();
        }

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(flags_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(link_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(media_addresses_.serialize(w));
            return nb::SerializeResult::Ok;
        }
    };
} // namespace net::neighbor::service
