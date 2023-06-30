#pragma once

#include "stream.h"

namespace nb::stream {
    template <typename... Ws>
    size_t writable_count(const Ws &...writers) {
        return (writers.writable_count() + ...);
    }

    template <typename... Rs>
    size_t readable_count(const Rs &...readers) {
        return (readers.readable_count() + ...);
    }

    template <typename... Ws>
    bool write(const uint8_t data, Ws &...writers) {
        return (writers.write(data) || ...);
    }

    template <typename... Rs>
    etl::optional<uint8_t> read(Rs &...readers) {
        etl::optional<uint8_t> result{};
        ((result = readers.read()) || ...);
        return result;
    }

    template <typename... Ts>
    bool is_closed(const Ts &...streams) {
        return (streams.is_closed() && ...);
    }
} // namespace nb::stream
