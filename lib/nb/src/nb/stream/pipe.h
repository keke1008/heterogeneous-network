#pragma once

#include "stream.h"

namespace nb::stream {
    template <typename R, typename W>
    etl::void_t<is_stream_reader_t<R>, is_stream_writer_t<W>> pipe(R &reader, W &writer) {
        while (reader.is_readable() && writer.is_writable()) {
            writer.write(*reader.read());
        }
    }

    template <typename... Rs, typename W>
    etl::enable_if_t<are_all_stream_reader_v<Rs...>, bool> pipe_readers(W &writer, Rs &...readers) {
        auto pipe_inner = [](auto &reader, auto &writer) {
            pipe(reader, writer);
            return writer.is_writable();
        };
        return !(pipe_inner(readers, writer) && ...);
    }

    template <typename R, typename... Ws>
    etl::enable_if_t<are_all_stream_writer_v<Ws...>, bool> pipe_writers(R &reader, Ws &...writers) {
        auto pipe_inner = [](auto &reader, auto &writer) {
            pipe(reader, writer);
            return reader.is_readable();
        };
        return !(pipe_inner(reader, writers) && ...);
    }
} // namespace nb::stream
