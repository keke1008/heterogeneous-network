#pragma once

#include "../address.h"
#include <debug_assert.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/span.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <serde/dec.h>
#include <util/span.h>

namespace net::link::wifi {
    class IPv4Address final : public nb::buf::BufferWriter {
        friend class IPv4PrettyBufferWriter;
        etl::array<uint8_t, 4> bytes_;

      public:
        constexpr explicit IPv4Address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
            : bytes_{b1, b2, b3, b4} {}

        explicit IPv4Address(etl::span<const uint8_t> bytes) {
            DEBUG_ASSERT(bytes.size() == 4);
            etl::copy(bytes.begin(), bytes.end(), bytes_.data());
        }

        explicit IPv4Address(const Address &address) {
            DEBUG_ASSERT(address.type() == AddressType::IPv4);
            DEBUG_ASSERT(address.address().size() == 4);
            bytes_.assign(address.address().begin(), address.address().end());
        }

        bool operator==(const IPv4Address &other) const {
            return bytes_ == other.bytes_;
        }

        bool operator!=(const IPv4Address &other) const {
            return bytes_ != other.bytes_;
        }

        explicit operator Address() const {
            return Address{AddressType::IPv4, bytes_};
        }

        void write_to_builder(nb::buf::BufferBuilder &builder) override {
            DEBUG_ASSERT(builder.writable_count() >= 15);

            auto write_byte = [](uint8_t byte) {
                return [byte](auto span) { return serde::dec::serialize(span, byte); };
            };

            for (uint8_t i = 0; i < 3; ++i) {
                builder.append(write_byte(bytes_[i]));
                builder.append(static_cast<uint8_t>('.'));
            }
            builder.append(write_byte(bytes_[3]));
        }
    };

    struct IPv4AddressParser {
        inline IPv4Address parse(nb::buf::BufferSplitter &splitter) {
            etl::array<uint8_t, 4> address{0, 0, 0, 0};
            for (uint8_t i = 0; i < 3; i++) {
                auto part = splitter.split_sentinel('.');
                address[i] = serde::dec::deserialize<uint8_t>(part);
            }
            auto part = splitter.split_remaining();
            address[3] = serde::dec::deserialize<uint8_t>(part);
            return IPv4Address{address};
        }
    };

    struct IPv4AddressWithTrailingCommaParser {
        inline IPv4Address parse(nb::buf::BufferSplitter &splitter) {
            nb::buf::BufferSplitter sub_splitter{splitter.split_sentinel(',')};
            return IPv4AddressParser{}.parse(sub_splitter);
        }
    };
} // namespace net::link::wifi
