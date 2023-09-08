#pragma once

#include "./types.h"
#include <etl/array.h>
#include <etl/functional.h>
#include <etl/initializer_list.h>
#include <etl/span.h>

namespace nb::stream {
    namespace _private_reader {
        inline void initialize_array_single(uint8_t **ptr, uint8_t value) {
            **ptr = value;
            ++*ptr;
        }

        inline void initialize_array_single(uint8_t **ptr, etl::span<uint8_t> values) {
            for (auto value : values) {
                **ptr = value;
                ++*ptr;
            }
        }

        inline void initialize_array_single(uint8_t **ptr, StreamReader &reader) {
            for (auto poll = reader.read(); poll.is_ready(); poll = reader.read()) {
                **ptr = poll.unwrap();
                ++*ptr;
            }
        }

        template <uint8_t N, typename... Rs>
        uint8_t initialize_array(etl::array<uint8_t, N> &array, Rs &&...rs) {
            uint8_t *end = array.data();
            (_private_reader::initialize_array_single(&end, etl::forward<Rs>(rs)), ...);
            return end - array.data();
        }
    } // namespace _private_reader

    template <uint8_t MAX_LENGTH>
    class FixedStreamReader final : public FiniteStreamReader {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        uint8_t index_{0};

      public:
        FixedStreamReader() = delete;

        template <typename... Rs>
        FixedStreamReader(Rs &&...rs) {
            length_ = _private_reader::initialize_array<MAX_LENGTH, Rs...>(
                bytes_, etl::forward<Rs>(rs)...
            );
        }

        nb::Poll<uint8_t> read() override {
            if (index_ < length_) {
                return bytes_[index_++];
            } else {
                return nb::pending;
            }
        }

        nb::Poll<void> wait_until_empty() override {
            if (index_ < length_) {
                return nb::pending;
            } else {
                return nb::ready();
            }
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedStreamWriter final : public FiniteStreamWriter {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        uint8_t index_{0};

      public:
        FixedStreamWriter() : length_{MAX_LENGTH} {};
        FixedStreamWriter(uint8_t length) : length_{length} {};

      protected:
        inline nb::Poll<void> wait_until_writable() override {
            if (index_ < length_) {
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        inline void write(uint8_t byte) override {
            bytes_[index_++] = byte;
        }

      public:
        nb::Poll<etl::span<uint8_t>> poll() {
            if (index_ < length_) {
                return nb::pending;
            } else {
                return etl::span(bytes_.data(), length_);
            }
        }
    };
} // namespace nb::stream
