#include "./exception.h"

#include <etl/error_handler.h>

#if __has_include(<Arduino.h>)

#include <undefArduino.h>

namespace util {
    struct ErrorHandler {
        void handle(const etl::exception &e) const {
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
    };

    void handle(const etl::exception &e) {
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

    const ErrorHandler handler{};

    void set_error_handler() {
        // コンパイルフラッグに-fltoが渡されていると，この行でリンクエラーが発生する．
        etl::error_handler::set_callback<handle>();
    }
} // namespace util

#else

namespace util {
    void set_error_handler(const etl::exception &e) {}
} // namespace util

#endif
