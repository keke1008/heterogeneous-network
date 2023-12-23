#pragma once

#include "./assert.h"
#include "./halt.h"    // IWYU pragma: keep
#include "./handler.h" // IWYU pragma: keep
#include <etl/span.h>
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
    class Printer {
        HardwareSerial &serial_;

      public:
        explicit Printer(HardwareSerial &serial) : serial_{serial} {}

        template <typename T>
        Printer &operator<<(const T &value) {
            serial_.print(value);
            return *this;
        }

        Printer &operator<<(etl::string_view str) {
            serial_.write('"');
            for (auto c : str) {
                serial_.write(c);
            }
            serial_.write('"');
            return *this;
        }

        Printer &operator<<(etl::span<const uint8_t> buffer) {
            serial_.write('[');
            for (auto c : buffer) {
                serial_.print(static_cast<int>(c));
                serial_.write(", ");
            }
            serial_.write(']');
            return *this;
        }

        template <size_t N>
        Printer &operator<<(etl::span<const uint8_t, N> buffer) {
            return operator<<(etl::span<const uint8_t>{buffer});
        }

        void println() {
            serial_.println();
        }
    };

    inline void flush() {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            return;
        }

        handler->flush();
    }

    template <typename... Args>
    void log(LogLevel level, Args &&...args) {
        auto handler = handler::get_handler();
        if (handler == nullptr) {
            return;
        }

        Printer printer{*handler};
        printer << '[' << log_level_to_string(level) << "] ";
        (printer << ... << args);
        printer.println();
    }
}; // namespace logger::log

#else

namespace logger::log {
    class Printer {
      public:
        template <typename T>
        Printer &operator<<(const T &) {
            return *this;
        }
    };

    template <typename... Args>
    void log(LogLevel, Args &&...) {}

    inline void flush() {}
} // namespace logger::log

#endif

#ifdef RELEASE_BUILD

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_FLUSH() ((void)0)

#else

#define LOG_TRACE(...) logger::log::log(logger::log::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) logger::log::log(logger::log::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) logger::log::log(logger::log::LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) logger::log::log(logger::log::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) logger::log::log(logger::log::LogLevel::Error, __VA_ARGS__)
#define LOG_FLUSH() logger::log::flush()

#endif
