#include "./assert.h"

#include "./handler.h" // IWYU pragma: keep

namespace logger::assert {
    util::FlashStringType message_to_string(MessageType type) {
        switch (type) {
        case MessageType::Assert:
            return FLASH_STRING("assertion failed");
        case MessageType::Panic:
            return FLASH_STRING("panic");
        case MessageType::Unreachable:
            return FLASH_STRING("unreachable code");
        case MessageType::UnreachableDefaultCase:
            return FLASH_STRING("unreachable default case");
        default:
            UNREACHABLE_DEFAULT_CASE;
        }
    }
} // namespace logger::assert

#if __has_include(<Arduino.h>)

namespace logger::assert {
    [[noreturn]] void
    panic(MessageType type, const char *file_name, int line_number, const char *message) {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            halt();
        }

        handler->println(FLASH_STRING("PANIC: "));

        handler->print(FLASH_STRING("  type: "));
        handler->println(message_to_string(type));

        // handler->print(FLASH_STRING("  file: "));
        // handler->println(file_name);

        handler->print(FLASH_STRING("  line: "));
        handler->println(line_number);

        if (message != nullptr) {
            handler->print(FLASH_STRING("  message: "));
            handler->println(message);
        }

        halt();
    }
} // namespace logger::assert

#else

namespace logger::assert {
    [[noreturn]] void
    panic(MessageType type, const char *file_name, int line_number, const char *message) {
        halt();
    }
} // namespace logger::assert

#endif
