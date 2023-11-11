#pragma once

#include "./assert.h"
#include "./halt.h"
#include "./handler.h"
#include <etl/string_view.h>
#include <stdint.h>
#include <util/flash_string.h>

namespace logger::log {
    enum class LogLevel : uint8_t { Trace, Debug, Info, Warning, Error };

    inline auto log_level_to_string(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:
            return FLASH_STRING("TRACE");
        case LogLevel::Debug:
            return FLASH_STRING("DEBUG");
        case LogLevel::Info:
            return FLASH_STRING("INFO");
        case LogLevel::Warning:
            return FLASH_STRING("WARNING");
        case LogLevel::Error:
            return FLASH_STRING("ERROR");
        default:
            UNREACHABLE_DEFAULT_CASE;
        }
    }

} // namespace logger::log

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logger::log {
    inline void print(HardwareSerial &serial, etl::string_view str) {
        for (auto c : str) {
            serial.write(c);
        }
    }

    template <typename T>
    inline void print(HardwareSerial &serial, T &value) {
        serial.print(value);
    }

    template <typename... Args>
    void log(LogLevel level, Args... args) {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            return;
        }

        handler->print('[');
        handler->print(log_level_to_string(level));
        handler->print(']');
        handler->print(' ');
        (print(*handler, args), ...);
        handler->println();
    }
}; // namespace logger::log

#elif __has_include(<doctest.h>)
#include <doctest.h>
#include <sstream>

namespace logger::log {
    inline void print(std::stringstream &ss, etl::string_view str) {
        for (auto c : str) {
            ss << c;
        }
    }

    template <typename T>
    inline void print(std::stringstream &ss, T &value) {
        ss << value;
    }

    template <typename... Args>
    void log(LogLevel level, Args... args) {
        INFO('[', log_level_to_string(level), "] ");

        std::stringstream ss;
        (print(ss, args), ...);
        INFO(doctest::String(ss, ss.str().size()));
    }
} // namespace logger::log

#else

namespace logger::log {
    template <typename... Args>
    void log(LogLevel, Args...) {}
} // namespace logger::log

#endif

#ifdef RELEASE_BUILD

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_ERROR(...) ((void)0)

#else

#define LOG_TRACE(...) logger::log::log(logger::log::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) logger::log::log(logger::log::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) logger::log::log(logger::log::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) logger::log::log(logger::log::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) logger::log::log(logger::log::LogLevel::Error, __VA_ARGS__)

#endif
