#pragma once

#include "./halt.h"
#include "./handler.h"
#include <etl/error_handler.h>

#if __has_include(<Arduino.h>)
#ifdef RELEASE_BUILD

namespace logger::exception {
    inline void register_exception_handler() {}
} // namespace logger::exception

#else
#include <undefArduino.h>

namespace logger::exception {
    static void handler(const etl::exception &e) {
        auto exception_handler = handler::get_handler();
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

    inline void register_exception_handler() {
        etl::error_handler::set_callback<handler>();
    }
} // namespace logger::exception

#endif
#endif
