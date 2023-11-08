#pragma once

#include <etl/error_handler.h>

namespace tl {
    class Panic : public etl::exception {
      public:
        Panic(string_type file_name, numeric_type line_number, string_type message)
            : etl::exception(message, file_name, line_number) {}
    };
}; // namespace tl

#define PANIC(message)                                                                             \
    ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Program panicked with message: " message))

#define ASSERT(condition)                                                                          \
    ETL_ASSERT(condition, ETL_ERROR_WITH_VALUE(tl::Panic, "Assertion failed: " #condition))

#define UNREACHABLE(message)                                                                       \
    ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unreachable code. " message))

#define UNREACHABLE_DEFAULT_CASE                                                                   \
    ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unreachable default case"))

#define UNIMPLEMENTED(message)                                                                     \
    ETL_ASSERT_FAIL(ETL_ERROR_WITH_VALUE(tl::Panic, "Reached unimplemented code. " message))
