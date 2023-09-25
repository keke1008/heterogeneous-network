#pragma once

#include "./generic.h"
#include <etl/string_view.h>
#include <nb/poll.h>

namespace net::link::wifi {
    extern const etl::array<etl::string_view, 3> COMMANDS;

    class Intialization {
        using Command = nb::stream::FixedReadableBuffer<32>;

        nb::Promise<bool> promise_;
        Command command_buffer_{COMMANDS[0]};
        uint8_t command_index_ = 0;
        nb::stream::MaxLengthSingleLineWrtableBuffer<7> response_; // "ERROR\r\n" or "OK\r\n"で7

      public:
        explicit Intialization(nb::Promise<bool> &&promise) : promise_{etl::move(promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (command_index_ >= COMMANDS.size()) {
                return nb::ready();
            }

            while (true) {
                POLL_UNWRAP_OR_RETURN(command_buffer_.read_all_into(stream));
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));

                auto line = POLL_UNWRAP_OR_RETURN(response_.poll());
                if (Response<ResponseType::ERROR>::try_parse(line)) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                if (!Response<ResponseType::OK>::try_parse(line)) {
                    continue;
                }

                command_index_++;
                if (command_index_ >= COMMANDS.size()) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                command_buffer_ = Command{COMMANDS[command_index_]};
                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
