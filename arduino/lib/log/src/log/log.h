#pragma once

#include "./assert.h"
#include <stdint.h>

namespace logging::log {
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
} // namespace logging::log

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logging::log {
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
}; // namespace logging::log

#elif __has_include(<doctest.h>)
#include <doctest.h>
#include <sstream>

namespace logging::log {
    template <typename T>
    void register_log_handler(T &) {}

    template <typename... Args>
    void log(LogLevel level, Args... args) {
        MESSAGE(log_level_to_string(level));

        std::stringstream ss;
        (ss << ... << args);
        MESSAGE(doctest::String(ss, ss.str().size()));
    }
} // namespace logging::log

#else

namespace logging::log {
    template <typename T>
    void register_log_handler(T &) {}

    template <typename... Args>
    void log(LogLevel, Args...) {}
} // namespace logging::log

#endif

#ifdef RELEASE_BUILD

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_ERROR(...) ((void)0)

#else

#define LOG_TRACE(...) logging::log::log(logging::log::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) logging::log::log(logging::log::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) logging::log::log(logging::log::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) logging::log::log(logging::log::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) logging::log::log(logging::log::LogLevel::Error, __VA_ARGS__)

#endif
