#include "./types.h"

namespace nb::stream {
    nb::Poll<void> nb::stream::StreamReader::fill_single(StreamWriter<Item> &writer) {
        while (writer.wait_until_writable().is_ready()) {
            writer.write(POLL_UNWRAP_OR_RETURN(read()));
        }
        return nb::ready();
    }
} // namespace nb::stream
