#pragma once

#include "./halt.h"
#include <etl/error_handler.h>

namespace logging {
    class Panic : public etl::exception {
      public:
        Panic(string_type file_name, numeric_type line_number, string_type message)
            : etl::exception(message, file_name, line_number) {}
    };
} // namespace logging

#ifdef RELEASE_BUILD

#define ASSERT(condition) ((void)0)
#define PANIC(message) logging::halt()
#define UNREACHABLE(message) logging::halt()
#define UNREACHABLE_DEFAULT_CASE logging::halt()

#else

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        ETL_ASSERT(                                                                                \
            condition, ETL_ERROR_WITH_VALUE(logging::Panic, "Assertion failed: " #condition)       \
        );                                                                                         \
    } while (false)

#define PANIC(message)                                                                             \
    do {                                                                                           \
        ETL_ASSERT_FAIL(                                                                           \
            ETL_ERROR_WITH_VALUE(logging::Panic, "Program panicked with message: " message)        \
        );                                                                                         \
        logging::halt();                                                                           \
    } while (false)

#define UNREACHABLE(message)                                                                       \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(logging::Panic, "Reached unreachable code. " message) \
        );                                                                                         \
        logging::halt();                                                                           \
    } while (false)

#define UNREACHABLE_DEFAULT_CASE                                                                   \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(logging::Panic, "Reached unreachable default case")); \
        logging::halt();                                                                           \
    } while (false)

#endif
