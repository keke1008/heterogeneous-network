#pragma once

#include <debug_assert.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/span.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <serde/dec.h>
#include <util/span.h>

namespace net::link {
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

        bool operator==(const IPv4Address &other) const {
            return bytes_ == other.bytes_;
        }

        bool operator!=(const IPv4Address &other) const {
            return bytes_ != other.bytes_;
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

      public:
        static IPv4Address deserialize_from_pretty_format(etl::span<const uint8_t> bytes) {
            DEBUG_ASSERT(bytes.size() <= 15);

            IPv4Address address{0, 0, 0, 0};
            for (uint8_t i = 0; i < 3; i++) {
                auto part = util::span::split_until_byte_exclusive(bytes, '.');
                DEBUG_ASSERT(part.has_value());
                address.bytes_[i] = serde::dec::deserialize<uint8_t>(part.value());
            }
            address.bytes_[3] = serde::dec::deserialize<uint8_t>(bytes);
            return address;
        }
    };
} // namespace net::link
