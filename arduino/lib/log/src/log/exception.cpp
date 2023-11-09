#include "./exception.h"

#include "./halt.h"

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logging {
    void handler(const etl::exception &e) {
        Serial.println("Error occurred");

        Serial.print("message: ");
        Serial.println(e.what());

        Serial.print("file: ");
        Serial.println(e.file_name());

        Serial.print("line: ");
        Serial.println(e.line_number());

        Serial.flush();
        halt();
    }

    void registre_exception_handler() {
        etl::error_handler::set_callback<handler>();
    }
} // namespace logging

#else

namespace logging {
    void registre_exception_handler() {}
} // namespace logging

#endif
