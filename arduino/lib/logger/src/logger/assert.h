#pragma once

#include "./halt.h"
#include <util/flash_string.h>

namespace logger::assert {
    enum class MessageType {
        Assert,
        Panic,
        Unreachable,
        UnreachableDefaultCase,
    };

    [[noreturn]] void
    panic(MessageType type, const char *file_name, int line_number, const char *message = nullptr);
} // namespace logger::assert

#ifdef RELEASE_BUILD

#define ASSERT(condition) ((void)0)
#define PANIC(message) logger::halt()
#define UNREACHABLE(message) logger::halt()
#define UNREACHABLE_DEFAULT_CASE logger::halt()

#else

#define LOGGER_PANIC_HELPER(type, message)                                                         \
    logger::assert::panic(logger::assert::MessageType::type, __FILE__, __LINE__, message);

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            LOGGER_PANIC_HELPER(Assert, "assertion failed: " #condition);                          \
        }                                                                                          \
    } while (false)

#define PANIC(message) LOGGER_PANIC_HELPER(Panic, message)
#define UNREACHABLE(message) LOGGER_PANIC_HELPER(Unreachable, message)
#define UNREACHABLE_DEFAULT_CASE                                                                   \
    logger::assert::panic(logger::assert::MessageType::UnreachableDefaultCase, __FILE__, __LINE__);

#endif
