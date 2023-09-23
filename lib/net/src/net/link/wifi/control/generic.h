#pragma once

#include "./response.h"
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>

namespace net::link::wifi {
    static constexpr etl::string_view CRLF{"\r\n"};

    template <
        uint8_t COMMAND_BUFFER_SIZE,
        ResponseType SUCCESS_RESPONSE,
        ResponseType FAILURE_RESPONSE>
    class Control {
        bool done_{false};
        nb::stream::FixedReadableBuffer<COMMAND_BUFFER_SIZE> command_;
        ExactMatchResponse<SUCCESS_RESPONSE, FAILURE_RESPONSE> response_;

      public:
        template <typename... Args>
        explicit Control(nb::Promise<bool> &&promise, Args &&...args)
            : command_{etl::forward<Args>(args)...},
              response_{etl::move(promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (!done_) {
                POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream));
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                done_ = true;
            }
            return nb::ready();
        }
    };
} // namespace net::link::wifi
