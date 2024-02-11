#pragma once

#include "./generic.h"
#include <etl/string_view.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>

namespace media::wifi {
    inline const etl::array<etl::string_view, 3> COMMANDS{
        "AT+CIPMUX=0\r\n",   // Disable multiple connections
        "AT+CWMODE=1\r\n",   // Set station mode
        "AT+CIPDINFO=1\r\n", // Show connection info

    };

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    class Initialization {
        memory::Static<W> &writable_;
        nb::Promise<bool> promise_;

        uint8_t next_index_{0};
        GenericEmptyResponseSyncControl<R, W, nb::ser::AsyncStaticSpanSerializer> control_;

      public:
        explicit Initialization(memory::Static<W> &writable, nb::Promise<bool> &&promise)
            : writable_{writable},
              promise_{etl::move(promise)},
              control_{writable, WifiResponseMessage::Ok, COMMANDS[next_index_++]} {}

        nb::Poll<void> execute() {
            if (next_index_ >= COMMANDS.size()) {
                return nb::ready();
            }

            while (true) {
                auto &&success = POLL_MOVE_UNWRAP_OR_RETURN(control_.execute());
                if (!success) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                next_index_++;
                if (next_index_ >= COMMANDS.size()) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                control_ =
                    GenericEmptyResponseSyncControl<R, W, nb::ser::AsyncStaticSpanSerializer>{
                        writable_, WifiResponseMessage::Ok, COMMANDS[next_index_]
                    };
            }
        }

        void handle_message(WifiMessage<R> &&message) {
            control_.handle_message(etl::move(message));
        }
    };
} // namespace media::wifi
