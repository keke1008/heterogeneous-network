#pragma once

#include "./halt.h" // IWYU pragma: export
#include <util/flash_string.h>

namespace logger::assert {
    enum class MessageType {
        Assert,
        Panic,
        Unreachable,
        UnreachableDefaultCase,
    };

    [[noreturn]] void panic(MessageType type, int line_number, const char *message = nullptr);
    [[noreturn]] void panic(MessageType type, int line_number, util::FlashStringType message);
} // namespace logger::assert

#ifdef RELEASE_BUILD

#define ASSERT(condition) ((void)0)
#define PANIC(message) logger::halt()
#define UNREACHABLE(message) logger::halt()
#define UNREACHABLE_DEFAULT_CASE logger::halt()

#define FASSERT(condition) ((void)0)
#define FPANIC(message) logger::halt()
#define FUNREACHABLE(message) logger::halt()

#else

#define LOGGER_PANIC_HELPER(type, message)                                                         \
    logger::assert::panic(logger::assert::MessageType::type, __LINE__, message);
#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            /* LOGGER_PANIC_HELPER(Assert, "assertion failed: " #condition); */                    \
            LOGGER_PANIC_HELPER(Assert, #condition);                                               \
        }                                                                                          \
    } while (false)
#define PANIC(message) LOGGER_PANIC_HELPER(Panic, message)
#define UNREACHABLE(message) LOGGER_PANIC_HELPER(Unreachable, message)
#define UNREACHABLE_DEFAULT_CASE                                                                   \
    logger::assert::panic(logger::assert::MessageType::UnreachableDefaultCase, __LINE__);

#define LOGGER_FPANIC_HELPER(type, message)                                                        \
    logger::assert::panic(logger::assert::MessageType::type, __LINE__, FLASH_STRING(message));

#define FASSERT(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            LOGGER_FPANIC_HELPER(Assert, #condition);                                              \
        }                                                                                          \
    } while (false)
#define FPANIC(message) LOGGER_FPANIC_HELPER(Panic, message)
#define FUNREACHABLE(message) LOGGER_FPANIC_HELPER(Unreachable, message)

#endif
