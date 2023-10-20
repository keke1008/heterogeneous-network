#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/stream.h>

namespace net::link::uhf {
    class SNExecutor {
        FixedExecutor<0, 9> executor_;

      public:
        SNExecutor() : executor_{'@', 'S', 'N', '\r', '\n'} {}

        nb::Poll<SerialNumber> poll(nb::stream::ReadableWritableStream &stream) {
            const auto span = POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            return nb::ready(SerialNumber{span});
        }
    };
} // namespace net::link::uhf
