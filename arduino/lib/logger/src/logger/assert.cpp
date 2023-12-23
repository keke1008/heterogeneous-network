#include "./assert.h"

#include "./handler.h" // IWYU pragma: keep

namespace logger::assert {
    util::FlashStringType message_to_string(MessageType type) {
        switch (type) {
        case MessageType::Assert:
            return FLASH_STRING("Assertion failed");
        case MessageType::Panic:
            return FLASH_STRING("Panic");
        case MessageType::Unreachable:
            return FLASH_STRING("Unreachable code");
        case MessageType::UnreachableDefaultCase:
            return FLASH_STRING("Unreachable default case");
        default:
            UNREACHABLE_DEFAULT_CASE;
        }
    }
} // namespace logger::assert

#if __has_include(<Arduino.h>)

namespace logger::assert {
    void print_panic_header(HardwareSerial &handler, MessageType type, int line_number) {
        handler.println(message_to_string(type));

        handler.print(FLASH_STRING("  line: "));
        handler.println(line_number);
    }

    [[noreturn]] void panic(MessageType type, int line_number, const char *message) {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            halt();
        }

        print_panic_header(*handler, type, line_number);

        if (message != nullptr) {
            handler->print(FLASH_STRING("  message: "));
            handler->println(message);
        }

        halt();
    }

    [[noreturn]] void panic(MessageType type, int line_number, util::FlashStringType message) {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            halt();
        }

        print_panic_header(*handler, type, line_number);

        handler->print(FLASH_STRING("  message: "));
        handler->println(message);

        halt();
    }
} // namespace logger::assert

#else

namespace logger::assert {
    [[noreturn]] void panic(MessageType type, int line_number, const char *message) {
        halt();
    }

    [[noreturn]] void panic(MessageType type, int line_number, util::FlashStringType message) {
        halt();
    }
} // namespace logger::assert

#endif
