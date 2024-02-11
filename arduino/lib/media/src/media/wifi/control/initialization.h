#pragma once

#include "./generic.h"
#include <etl/string_view.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>

namespace media::wifi {
    inline constexpr uint8_t COMMANDS_SIZE = 3;

    inline util::FlashStringType command_at(uint8_t index) {
        static const etl::array<util::FlashStringType, COMMANDS_SIZE> COMMANDS{
            FLASH_STRING("AT+CIPMUX=0\r\n"),   // Disable multiple connections
            FLASH_STRING("AT+CWMODE=1\r\n"),   // Set station mode
            FLASH_STRING("AT+CIPDINFO=1\r\n"), // Show connection info
        };
        return COMMANDS[index];
    }

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    class Initialization {
        memory::Static<W> &writable_;
        nb::Promise<bool> promise_;

        uint8_t current_index_{0};
        GenericEmptyResponseSyncControl<R, W, nb::ser::AsyncFlashStringSerializer> control_;

      public:
        explicit Initialization(memory::Static<W> &writable, nb::Promise<bool> &&promise)
            : writable_{writable},
              promise_{etl::move(promise)},
              control_{writable, WifiResponseMessage::Ok, command_at(current_index_)} {}

        nb::Poll<void> execute() {
            if (current_index_ >= COMMANDS_SIZE) {
                return nb::ready();
            }

            while (true) {
                auto &&success = POLL_MOVE_UNWRAP_OR_RETURN(control_.execute());
                if (!success) {
                    promise_.set_value(false);
                    return nb::ready();
                }

                current_index_++;
                if (current_index_ >= COMMANDS_SIZE) {
                    promise_.set_value(true);
                    return nb::ready();
                }

                control_ =
                    GenericEmptyResponseSyncControl<R, W, nb::ser::AsyncFlashStringSerializer>{
                        writable_, WifiResponseMessage::Ok, command_at(current_index_)
                    };
            }
        }

        void handle_message(WifiMessage<R> &&message) {
            control_.handle_message(etl::move(message));
        }
    };
} // namespace media::wifi
