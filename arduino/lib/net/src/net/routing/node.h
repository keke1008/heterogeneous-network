#pragma once

#include <nb/buf.h>
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
} // namespace net::routing
