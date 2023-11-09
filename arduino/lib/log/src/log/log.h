#pragma once

#include "./assert.h"
#include <stdint.h>

namespace logging {
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
} // namespace logging

#if __has_include(<Arduino.h>)
#include <undefArduino.h>

namespace logging {
    template <typename... Args>
    void log(LogLevel level, Args... args) {
        Serial.print("[");
        Serial.print(log_level_to_string(level));
        Serial.print("] ");
        (Serial.print(args), ...);
        Serial.println();
    }
}; // namespace logging

#elif __has_include(<doctest.h>)
#include <doctest.h>
#include <sstream>

namespace logging {
    template <typename... Args>
    void log(LogLevel level, Args... args) {
        MESSAGE(log_level_to_string(level));

        std::stringstream ss;
        (ss << ... << args);
        MESSAGE(doctest::String(ss, ss.str().size()));
    }
} // namespace logging

#else

namespace logging {
    template <typename... Args>
    void log(LogLevel, Args...) {}
} // namespace logging

#endif

#ifdef RELEASE_BUILD

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_ERROR(...) ((void)0)

#else

#define LOG_TRACE(...) logging::log(logging::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) logging::log(logging::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) logging::log(logging::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) logging::log(logging::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) logging::log(logging::LogLevel::Error, __VA_ARGS__)

#endif
