#pragma once

#include "./halt.h"
#include <etl/error_handler.h>

namespace logger {
    class Panic : public etl::exception {
      public:
        Panic(string_type file_name, numeric_type line_number, string_type message)
            : etl::exception(message, file_name, line_number) {}
    };
} // namespace logger

#ifdef RELEASE_BUILD

#define ASSERT(condition) ((void)0)
#define PANIC(message) logger::halt()
#define UNREACHABLE(message) logger::halt()
#define UNREACHABLE_DEFAULT_CASE logger::halt()

#else

#if __has_include(<Arduino.h>)
#define LOGGING_CREATE_MESSAGE(title, message) (title)
#else
#define LOGGING_CREATE_MESSAGE(title, message) (title ": " message)
#endif

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        ETL_ASSERT(                                                                                \
            condition,                                                                             \
            ETL_ERROR_WITH_VALUE(                                                                  \
                logger::Panic, LOGGING_CREATE_MESSAGE("Assertion failed: ", #condition)            \
            )                                                                                      \
        );                                                                                         \
    } while (false)

#define PANIC(message)                                                                             \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(                                                      \
            logger::Panic, LOGGING_CREATE_MESSAGE("Program panicked with message: ", message)      \
        ));                                                                                        \
        logger::halt();                                                                            \
    } while (false)

#define UNREACHABLE(message)                                                                       \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(                                                      \
            logger::Panic, LOGGING_CREATE_MESSAGE("Reached unreachable code. ", message)           \
        ));                                                                                        \
        logger::halt();                                                                            \
    } while (false)

#define UNREACHABLE_DEFAULT_CASE                                                                   \
    do {                                                                                           \
        ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(logger::Panic, "Reached unreachable default case"));  \
        logger::halt();                                                                            \
    } while (false)

#endif
