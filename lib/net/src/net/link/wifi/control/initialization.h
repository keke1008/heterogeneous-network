#pragma once

#include "./generic.h"
#include <etl/string_view.h>
#include <nb/poll.h>

namespace net::link::wifi {
    extern const etl::array<etl::string_view, 5> COMMANDS;

    class Intialization {
        using Command = nb::stream::FixedReadableBuffer<32>;
        using Response = ExactMatchResponse<ResponseType::OK, ResponseType::ERROR>;

        nb::Promise<bool> promise_;
        Command command_buffer_{COMMANDS[0]};
        uint8_t command_index_ = 0;

        nb::Future<bool> control_result_;
        Response response_;

        explicit Intialization(
            nb::Promise<bool> &&promise,
            nb::Future<bool> &&control_result_future,
            nb::Promise<bool> &&control_result_promise
        )
            : promise_{etl::move(promise)},
              control_result_{etl::move(control_result_future)},
              response_{etl::move(control_result_promise)} {}

      public:
        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (command_index_ >= COMMANDS.size()) {
                return nb::ready();
            }

            while (true) {
                POLL_UNWRAP_OR_RETURN(command_buffer_.read_all_into(stream));
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                if (!POLL_UNWRAP_OR_RETURN(control_result_.poll())) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                command_index_++;
                if (command_index_ >= COMMANDS.size()) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                auto [future, promise] = nb::make_future_promise_pair<bool>();
                command_buffer_ = Command{COMMANDS[command_index_]};
                control_result_ = etl::move(future);
                response_ = Response{etl::move(promise)};
            }
        }
    };
} // namespace net::link::wifi
