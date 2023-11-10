#pragma once

#include "./assert.h"
#include <stdint.h>

namespace logger::log {
    enum class LogLevel : uint8_t { Trace, Debug, Info, Warning, Error };

    inline const char *log_level_to_string(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Error:
            return "ERROR";
        default:
            UNREACHABLE_DEFAULT_CASE;
        }
    }
} // namespace logger::log

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logger::log {
    static HardwareSerial *log_handler = nullptr;

    inline void register_log_handler(HardwareSerial &serial) {
        log_handler = &serial;
    }

    template <typename... Args>
    void log(LogLevel level, Args... args) {
        if (log_handler == nullptr) {
            return;
        }

        log_handler->print("[");
        log_handler->print(log_level_to_string(level));
        log_handler->print("] ");
        (log_handler->print(args), ...);
        log_handler->println();
    }
}; // namespace logger::log

#elif __has_include(<doctest.h>)
#include <doctest.h>
#include <sstream>

namespace logger::log {
    template <typename T>
    void register_log_handler(T &) {}

    template <typename... Args>
    void log(LogLevel level, Args... args) {
        MESSAGE(log_level_to_string(level));

        std::stringstream ss;
        (ss << ... << args);
        MESSAGE(doctest::String(ss, ss.str().size()));
    }
} // namespace logger::log

#else

namespace logger::log {
    template <typename T>
    void register_log_handler(T &) {}

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
