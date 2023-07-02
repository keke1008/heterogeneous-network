#pragma once

#include "./batch.h"
#include "./empty.h"
#include "./stream.h"
#include <etl/functional.h>
#include <util/tuple.h>

namespace nb::stream {
    template <typename... Ws>
    class TupleStreamWriter {
        static_assert(are_all_stream_writer_v<Ws...>);

        util::Tuple<Ws...> writers_;

      public:
        inline constexpr TupleStreamWriter() = default;

        template <typename... Ts>
        inline constexpr TupleStreamWriter(Ts &&...writers)
            : writers_{etl::forward<Ts>(writers)...} {}

        inline constexpr bool is_writable() const {
            return writers_.last().is_writable();
        }

        inline constexpr size_t writable_count() const {
            return util::apply(
                [](auto &...ws) {
                    return nb::stream::writable_count(etl::forward<decltype(ws)>(ws)...);
                },
                writers_
            );
        }

        inline constexpr bool write(const uint8_t &data) {
            return util::apply(
                [data](auto &&...ws) {
                    return nb::stream::write(data, etl::forward<decltype(ws)>(ws)...);
                },
                writers_
            );
        }

        inline constexpr bool is_writer_closed() const {
            return writers_->last().is_writer_closed();
        }
    };

    static_assert(is_stream_writer_v<TupleStreamWriter<EmptyStreamWriter, EmptyStreamWriter>>);

    template <typename... Rs>
    class TupleStreamReader {
        static_assert(are_all_stream_reader_v<Rs...>);
        util::Tuple<Rs...> readers_;

      public:
        TupleStreamReader() = default;

        template <typename... Ts>
        TupleStreamReader(Ts &&...readers) : readers_{etl::forward<Ts>(readers)...} {}

        bool is_readable() const {
            return readers_.last().is_readable();
        }

        size_t readable_count() const {
            return util::apply(
                [](auto &&...rs) {
                    return nb::stream::readable_count(etl::forward<decltype(rs)>(rs)...);
                },
                readers_
            );
        }

        etl::optional<uint8_t> read() {
            return util::apply(
                [](auto &&...rs) { return nb::stream::read(etl::forward<decltype(rs)>(rs)...); },
                readers_
            );
        }

        bool is_reader_closed() const {
            return this->last().is_reader_closed();
        }
    };

    static_assert(is_stream_reader_v<TupleStreamReader<EmptyStreamReader, EmptyStreamReader>>);
} // namespace nb::stream
