#pragma once

#include "./empty.h"
#include "./stream.h"
#include <etl/utility.h>
#include <etl/variant.h>

namespace nb::stream {
    template <typename... Ws>
    class VariantStreamWriter {
        static_assert(are_all_stream_writer_v<Ws...>);

        etl::variant<Ws...> writer_;

      public:
        template <typename... Ts>
        inline constexpr VariantStreamWriter(Ts &&...ts) : writer_{etl::forward<Ts>(ts)...} {}

        inline constexpr bool is_writable() const {
            return etl::visit([](const auto &writer) { return writer.is_writable(); }, writer_);
        }

        inline constexpr size_t writable_count() const {
            return etl::visit([](const auto &writer) { return writer.writable_count(); }, writer_);
        }

        inline constexpr bool write(const uint8_t data) {
            return etl::visit([data](auto &writer) { return writer.write(data); }, writer_);
        }

        inline constexpr bool is_writer_closed() const {
            return etl::visit(
                [](const auto &writer) { return writer.is_writer_closed(); }, writer_
            );
        }
    };

    static_assert(is_stream_writer_v<VariantStreamWriter<EmptyStreamWriter, EmptyStreamWriter>>);

    template <typename... Rs>
    class VariantStreamReader {
        static_assert(are_all_stream_reader_v<Rs...>);

        etl::variant<Rs...> reader_;

      public:
        template <typename... Ts>
        inline constexpr VariantStreamReader(Ts &&...ts) : reader_{etl::forward<Ts>(ts)...} {}

        inline constexpr bool is_readable() const {
            return etl::visit([](const auto &reader) { return reader.is_readable(); }, reader_);
        }

        inline constexpr size_t readable_count() const {
            return etl::visit([](const auto &reader) { return reader.readable_count(); }, reader_);
        }

        inline constexpr uint8_t read() {
            return etl::visit([](auto &reader) { return reader.read(); }, reader_);
        }

        inline constexpr bool is_reader_closed() const {
            return etl::visit(
                [](const auto &reader) { return reader.is_reader_closed(); }, reader_
            );
        }
    };

    static_assert(is_stream_reader_v<VariantStreamReader<EmptyStreamReader, EmptyStreamReader>>);
} // namespace nb::stream
