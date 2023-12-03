#pragma once

#include <nb/serde.h>
#include <net/link.h>

namespace net::node {
    class NodeId {
        friend class AsyncNodeIdDeserializer;
        friend class AsyncNodeIdSerializer;

        link::Address address_;

      public:
        static constexpr uint8_t MAX_LENGTH = 1 + link::Address::MAX_LENGTH;

        explicit NodeId(const link::Address &address) : address_(address) {}

        inline static NodeId broadcast() {
            return NodeId{link::Address::broadcast()};
        }

        inline bool is_broadcast() const {
            return address_.type() == link::AddressType::Broadcast;
        }

        inline bool operator==(const NodeId &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const NodeId &other) const {
            return address_ != other.address_;
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const NodeId &node_id) {
            return printer << node_id.address_;
        }
    };

    class AsyncNodeIdDeserializer {
        link::AsyncAddressDeserializer address_;

      public:
        inline NodeId result() const {
            return NodeId{address_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return address_.deserialize(r);
        }
    };

    class AsyncNodeIdSerializer {
        link::AsyncAddressSerializer address_;

      public:
        explicit AsyncNodeIdSerializer(const NodeId &node_id) : address_{node_id.address_} {}

        static constexpr inline uint8_t get_serialized_length(const NodeId &node_id) {
            return link::AsyncAddressSerializer::get_serialized_length(node_id.address_);
        }

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return address_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return address_.serialized_length();
        }
    };
} // namespace net::node
