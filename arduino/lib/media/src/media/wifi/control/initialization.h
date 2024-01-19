#pragma once

#include "../response.h"
#include <etl/string_view.h>
#include <nb/future.h>
#include <nb/poll.h>

namespace media::wifi {
    extern const etl::array<etl::string_view, 3> COMMANDS;

    class Initialization {
        nb::Promise<bool> promise_;
        nb::ser::AsyncStaticSpanSerializer command_{COMMANDS[0]};
        uint8_t command_index_ = 0;
        AsyncResponseTypeDeserializer response_;

      public:
        explicit Initialization(nb::Promise<bool> &&promise) : promise_{etl::move(promise)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            if (command_index_ >= COMMANDS.size()) {
                return nb::ready();
            }

            while (true) {
                POLL_UNWRAP_OR_RETURN(command_.serialize(rw));
                POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));

                auto result = response_.result();
                if (is_error_response(result)) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                command_index_++;
                if (command_index_ >= COMMANDS.size()) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                response_ = AsyncResponseTypeDeserializer{};
                command_ = nb::ser::AsyncStaticSpanSerializer{COMMANDS[command_index_]};
            }
        }
    };
} // namespace media::wifi
