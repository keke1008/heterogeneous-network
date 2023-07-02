#pragma once

#include "stream.h"

namespace nb::stream {
    template <typename... Ws>
    etl::enable_if_t<are_all_stream_writer_v<Ws...>, bool>
    write(const uint8_t data, Ws &...writers) {
        return (writers.write(data) || ...);
    }

    template <typename... Rs>
    etl::enable_if_t<are_all_stream_reader_v<Rs...>, etl::optional<uint8_t>> read(Rs &...readers) {
        etl::optional<uint8_t> result = etl::nullopt;
        ((result = readers.read()) || ...);
        return result;
    }

    template <typename... Ws>
    etl::enable_if_t<are_all_stream_writer_v<Ws...>, bool> is_writable(const Ws &...writers) {
        return (writers.is_writable() || ...);
    }

    template <typename... Rs>
    etl::enable_if_t<are_all_stream_reader_v<Rs...>, bool> is_readable(const Rs &...readers) {
        return (readers.is_readable() || ...);
    }

    template <typename... Ws>
    etl::enable_if_t<are_all_stream_writer_v<Ws...>, size_t> writable_count(const Ws &...writers) {
        return (writers.writable_count() + ...);
    }

    template <typename... Rs>
    etl::enable_if_t<are_all_stream_reader_v<Rs...>, size_t> readable_count(const Rs &...readers) {
        return (readers.readable_count() + ...);
    }

    template <typename... Rs>
    etl::enable_if_t<are_all_stream_reader_v<Rs...>, bool> is_reader_closed(const Rs &...streams) {
        return (streams.is_reader_closed() && ...);
    }

    template <typename... Ws>
    etl::enable_if_t<are_all_stream_reader_v<Ws...>, bool> is_writer_closed(const Ws &...streams) {
        return (streams.is_writer_closed() && ...);
    }
} // namespace nb::stream
