#include "./exception.h"

#include <etl/error_handler.h>

#if __has_include(<Arduino.h>)

#include <undefArduino.h>

namespace util {
    void handler(const etl::exception &e) {
        Serial.println("Error occurred");

        Serial.print("message: ");
        Serial.println(e.what());

        Serial.print("file: ");
        Serial.println(e.file_name());

        Serial.print("line: ");
        Serial.println(e.line_number());

        Serial.flush();
        abort();
    }

    void set_error_handler() {
        etl::error_handler::set_callback<handler>();
    }
} // namespace util

#else

namespace util {
    void handler(const etl::exception &e) {}
} // namespace util

#endif
