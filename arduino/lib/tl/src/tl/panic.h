#pragma once

#include <etl/error_handler.h>

namespace tl {
    class Panic : public etl::exception {
      public:
        Panic(string_type file_name, numeric_type line_number, string_type message)
            : etl::exception(message, file_name, line_number) {}
    };

    [[noreturn]] void halt();
}; // namespace tl

#define PANIC(message)                                                                             \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Program panicked with message: " message) \
        );                                                                                         \
        tl::halt();                                                                                \
    } while (false)

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        ETL_ASSERT(condition, ETL_ERROR_WITH_VALUE(tl::Panic, "Assertion failed: " #condition));   \
        tl::halt();                                                                                \
    } while (false)

#define UNREACHABLE(message)                                                                       \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unreachable code. " message));    \
        tl::halt();                                                                                \
    } while (false)

#define UNREACHABLE_DEFAULT_CASE                                                                   \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unreachable default case"));      \
        tl::halt();                                                                                \
    } while (false)

#define UNIMPLEMENTED(message)                                                                     \
    ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unimplemented code. " message))
