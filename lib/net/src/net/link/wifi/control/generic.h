#pragma once

#include "../message.h"
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>

namespace net::link::wifi {
    static constexpr etl::string_view CRLF{"\r\n"};

    template <
        uint8_t COMMAND_BUFFER_SIZE,
        message::ResponseType SUCCESS_RESPONSE,
        message::ResponseType FAILURE_RESPONSE>
    class Control {
        static inline constexpr uint8_t MAX_RESPONSE_LENGTH = etl::
            max(message::response_type_length<SUCCESS_RESPONSE>,
                message::response_type_length<FAILURE_RESPONSE>);

        nb::Promise<bool> promise_;
        nb::stream::FixedReadableBuffer<COMMAND_BUFFER_SIZE> command_;
        nb::stream::MaxLengthSingleLineWrtableBuffer<MAX_RESPONSE_LENGTH> response_;

      public:
        template <typename... Args>
        explicit Control(nb::Promise<bool> &&promise, Args &&...args)
            : promise_{etl::move(promise)},
              command_{etl::forward<Args>(args)...} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            while (true) {
                POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream));
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                auto line = POLL_UNWRAP_OR_RETURN(response_.poll());

                if (message::Response<SUCCESS_RESPONSE>::try_parse(line)) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                if (message::Response<FAILURE_RESPONSE>::try_parse(line)) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
