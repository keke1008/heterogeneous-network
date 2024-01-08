#pragma once

#include <nb/serde.h>
#include <net/link.h>

namespace net::node {
    enum class NodeIdType : uint8_t {
        // 特別な意味を持つID
        Broadcast = 0xff,

        // Address互換のID
        Serial = 0x01,
        UHF = 0x02,
        IPv4 = 0x03,
        WebSocket = 0x04,
    };

    constexpr inline NodeIdType from_address_type(link::AddressType type) {
        return static_cast<NodeIdType>(type);
    }

    constexpr inline bool is_valid_node_id_type(uint8_t type) {
        return type == static_cast<uint8_t>(NodeIdType::Broadcast) ||
            type == static_cast<uint8_t>(NodeIdType::Serial) ||
            type == static_cast<uint8_t>(NodeIdType::UHF) ||
            type == static_cast<uint8_t>(NodeIdType::IPv4) ||
            type == static_cast<uint8_t>(NodeIdType::WebSocket);
    }

    using AsyncNodeIdTypeDeserializer = nb::de::Enum<NodeIdType, is_valid_node_id_type>;
    using AsyncNodeIdTypeSerializer = nb::ser::Enum<NodeIdType>;

    constexpr inline uint8_t MAX_NODE_ID_BODY_LENGTH = link::MAX_ADDRESS_BODY_LENGTH;

    constexpr inline uint8_t get_body_length_of(NodeIdType type) {
        switch (type) {
        case NodeIdType::Broadcast:
            return 0;
        case NodeIdType::Serial:
            return get_address_body_length_of(link::AddressType::Serial);
        case NodeIdType::UHF:
            return get_address_body_length_of(link::AddressType::UHF);
        case NodeIdType::IPv4:
            return get_address_body_length_of(link::AddressType::IPv4);
        case NodeIdType::WebSocket:
            return get_address_body_length_of(link::AddressType::WebSocket);
        default:
            LOG_ERROR((uint8_t)type);
            UNREACHABLE_DEFAULT_CASE;
        }
    }

    class NodeId {
        friend class AsyncNodeIdDeserializer;
        friend class AsyncNodeIdSerializer;

        NodeIdType type_;
        etl::array<uint8_t, MAX_NODE_ID_BODY_LENGTH> body_;

        NodeId(NodeIdType type, etl::span<const uint8_t> body) : type_{type} {
            FASSERT(body.size() == get_body_length_of(type));
            body_.assign(body.begin(), body.end(), 0);
        }

        etl::span<const uint8_t> body() const {
            return etl::span{body_.begin(), get_body_length_of(type_)};
        }

      public:
        explicit NodeId(const link::Address &address)
            : NodeId{from_address_type(address.type()), address.body()} {}

        static inline NodeId broadcast() {
            return NodeId{NodeIdType::Broadcast, {}};
        }

        inline bool operator==(const NodeId &other) const {
            return type_ == other.type_ && etl::equal(body(), other.body());
        }

        inline bool operator!=(const NodeId &other) const {
            return !(*this == other);
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const NodeId &node_id) {
            return printer << (uint8_t)node_id.type_ << '(' << node_id.body() << ')';
        }

        inline bool is_broadcast() const {
            return type_ == NodeIdType::Broadcast;
        }
    };

    class AsyncNodeIdDeserializer {
        AsyncNodeIdTypeDeserializer type_{};
        nb::de::Array<nb::de::Bin<uint8_t>, MAX_NODE_ID_BODY_LENGTH> body_{
            nb::de::ARRAY_DUMMY_INITIAL_LENGTH
        };

      public:
        inline NodeId result() const {
            return NodeId{type_.result(), body_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            body_.set_length(get_body_length_of(type_.result()));
            return body_.deserialize(r);
        }
    };

    class AsyncNodeIdSerializer {
        AsyncNodeIdTypeSerializer type_;
        nb::ser::Array<nb::ser::Bin<uint8_t>, MAX_NODE_ID_BODY_LENGTH> body_;

      public:
        explicit AsyncNodeIdSerializer(const NodeId &node_id)
            : type_{node_id.type_},
              body_{node_id.body()} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            return body_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + body_.serialized_length();
        }

        static constexpr inline uint8_t max_serialized_length() {
            return 1 + MAX_NODE_ID_BODY_LENGTH;
        }
    };
} // namespace net::node
