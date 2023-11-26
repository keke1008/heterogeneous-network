#pragma once

#include "../common.h"
#include "./common.h"
#include <memory/pair_shared.h>
#include <nb/serde.h>

namespace net::link::uhf {
    class SNExecutor {
        AsyncFixedExecutor<nb::ser::Empty, 9> executor_;

      public:
        SNExecutor() : executor_{'S', 'N', nb::ser::Empty{}} {}

        template <nb::AsyncReadable R>
        nb::Poll<SerialNumber> poll(R &stream) {
            const auto span = POLL_UNWRAP_OR_RETURN(executor_.poll(stream));
            return nb::ready(SerialNumber{span});
        }
    };
} // namespace net::link::uhf
