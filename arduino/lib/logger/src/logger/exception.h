#pragma once

#include "./halt.h"
#include <etl/error_handler.h>

#ifdef RELEASE_BUILD
#define IS_RELEASE_BUILD 1
#else
#define IS_RELEASE_BUILD 0
#endif

#if __has_include(<Arduino.h>) && !IS_RELEASE_BUILD
#include <undefArduino.h>

namespace logger::exception {
    static HardwareSerial *exception_handler = nullptr;

    static void handler(const etl::exception &e) {
        if (exception_handler == nullptr) {
            return;
        }

        exception_handler->println("Error occurred");

        exception_handler->print("message: ");
        exception_handler->println(e.what());

        exception_handler->print("file: ");
        exception_handler->println(e.file_name());

        exception_handler->print("line: ");
        exception_handler->println(e.line_number());

        exception_handler->flush();
        halt();
    }

    inline void register_exception_handler(HardwareSerial &serial) {
        exception_handler = &serial;
        etl::error_handler::set_callback<handler>();
    }
} // namespace logger::exception

#else

namespace logger::exception {
    template <typename T>
    void register_exception_handler(T &) {}
} // namespace logger::exception

#endif

#undef IS_RELEASE_BUILD
