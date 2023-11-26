#pragma once

#include <nb/buf.h>
#include <nb/serde.h>
#include <net/link.h>

namespace net::routing {
    class NodeId {
        friend class AsyncNodeIdDeserializer;
        friend class AsyncNodeIdSerializer;

        link::Address address_;

      public:
        static constexpr uint8_t MAX_LENGTH = 1 + link::Address::MAX_LENGTH;

        explicit NodeId(const link::Address &address) : address_(address) {}

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

    class Cost {
        uint16_t value_;

      public:
        static constexpr uint8_t LENGTH = 2;

        inline explicit constexpr Cost(uint16_t value) : value_{value} {}

        inline constexpr bool operator<(const Cost &other) const {
            return value_ < other.value_;
        }

        inline constexpr bool operator>(const Cost &other) const {
            return value_ > other.value_;
        }

        inline constexpr bool operator<=(const Cost &other) const {
            return value_ <= other.value_;
        }

        inline constexpr bool operator>=(const Cost &other) const {
            return value_ >= other.value_;
        }

        inline constexpr bool operator==(const Cost &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const Cost &other) const {
            return value_ != other.value_;
        }

        inline constexpr Cost operator+(const Cost &other) const {
            return Cost{static_cast<uint16_t>(value_ + other.value_)};
        }

        inline constexpr uint16_t value() const {
            return value_;
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const Cost &cost) {
            return printer << '(' << cost.value_ << ')';
        }
    };

    class AsyncCostDeserializer {
        nb::de::Bin<uint16_t> value_;

      public:
        inline Cost result() const {
            return Cost{value_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return value_.deserialize(r);
        }
    };

    class AsyncCostSerializer {
        nb::ser::Bin<uint16_t> value_;

      public:
        explicit AsyncCostSerializer(const Cost &cost) : value_{cost.value()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return value_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return value_.serialized_length();
        }
    };
} // namespace net::routing
