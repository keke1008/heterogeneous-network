#pragma once

#include "stream.h"

namespace nb::stream {
    template <typename R, typename W>
    bool pipe(R &reader, W &writer) {
        static_assert(is_stream_reader_v<
                      R, typename etl::remove_reference_t<W>::WritableStreamItem>);
        static_assert(is_stream_writer_v<
                      W, typename etl::remove_reference_t<R>::ReadableStreamItem>);

        while (reader.readable_count() && writer.writable_count()) {
            writer.write(reader.read());
        }

        return reader.is_closed() || writer.is_closed();
    }

    template <typename... Rs, typename W>
    bool pipe_readers(W &writer, Rs &...readers) {
        return (pipe<Rs, W>(readers, writer) && ...);
    }

    template <typename R, typename... Ws>
    bool pipe_writers(R &reader, Ws &...writers) {
        return (pipe<R, Ws>(reader, writers) && ...);
    }
} // namespace nb::stream
