#pragma once

#include <nb/buf.h>
#include <nb/serde.h>
#include <net/link.h>

namespace net::routing {
    class NodeId {
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

        inline uint8_t serialized_length() const {
            return 1 + address_.total_length();
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(address_);
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const NodeId &node_id) {
            return printer << node_id.address_;
        }
    };

    struct NodeIdParser {
        NodeId parse(nb::buf::BufferSplitter &splitter) {
            return NodeId{
                splitter.parse<link::AddressDeserializer>(),
            };
        }
    };

    struct AsyncNodeIdParser {
        link::AsyncAddressParser address_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            return address_parser_.parse(splitter);
        }

        inline NodeId result() {
            return NodeId(address_parser_.result());
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

        inline constexpr uint8_t serialized_length() const {
            return 2;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(nb::buf::FormatBinary<uint16_t>{value_});
        }
    };

    struct CostParser {
        inline Cost parse(nb::buf::BufferSplitter &splitter) {
            return Cost{splitter.parse<nb::buf::BinParser<uint16_t>>()};
        }
    };

    class AsyncCostParser {
        etl::optional<Cost> result_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            if (result_.has_value()) {
                return nb::ready();
            }
            result_ = POLL_UNWRAP_OR_RETURN(splitter.split_1byte());
            return nb::ready();
        }

        inline Cost result() {
            return result_.value();
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
} // namespace net::routing
