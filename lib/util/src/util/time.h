#pragma once

#include <stdint.h>

namespace util {
    using TimeInt = unsigned long;

    class Chrono;

    class Duration {
        TimeInt ms_;

        inline constexpr Duration(TimeInt ms) : ms_{ms} {}

      public:
        Duration() = delete;

        static inline constexpr Duration from_millis(TimeInt ms) {
            return Duration{ms};
        }

        static inline constexpr Duration from_seconds(TimeInt s) {
            return Duration{1000 * s};
        }

        inline constexpr Duration operator+(const Duration &rhs) const {
            return Duration{ms_ + rhs.ms_};
        }

        inline constexpr Duration &operator+=(const Duration &rhs) {
            ms_ += rhs.ms_;
            return *this;
        }

        inline constexpr Duration operator-(const Duration &rhs) const {
            return Duration{ms_ - rhs.ms_};
        }

        inline constexpr Duration &operator-=(const Duration &rhs) {
            ms_ -= rhs.ms_;
            return *this;
        }

        template <typename Int>
        inline constexpr Duration operator*(Int rhs) const {
            return Duration{ms_ * rhs};
        }

        template <typename Int>
        inline constexpr Duration &operator*=(Int rhs) {
            ms_ *= rhs;
            return *this;
        }

        template <typename Int>
        inline constexpr Duration operator/(Int rhs) const {
            return Duration{ms_ / rhs};
        }

        template <typename Int>
        inline constexpr Duration &operator/=(Int rhs) {
            ms_ /= rhs;
            return *this;
        }

        inline constexpr bool operator==(const Duration &rhs) const {
            return ms_ == rhs.ms_;
        }

        inline constexpr bool operator!=(const Duration &rhs) const {
            return ms_ != rhs.ms_;
        }

        inline constexpr bool operator<(const Duration &rhs) const {
            return ms_ < rhs.ms_;
        }

        inline constexpr bool operator<=(const Duration &rhs) const {
            return ms_ <= rhs.ms_;
        }

        inline constexpr bool operator>(const Duration &rhs) const {
            return ms_ > rhs.ms_;
        }

        inline constexpr bool operator>=(const Duration &rhs) const {
            return ms_ >= rhs.ms_;
        }

        inline constexpr TimeInt millis() const {
            return ms_;
        }

        inline constexpr TimeInt seconds() const {
            return ms_ / 1000;
        }
    };

    class Instant {
        friend class Time;
        friend class MockTime;

        TimeInt ms_;

        inline constexpr Instant(TimeInt ms) : ms_{ms} {}

      public:
        Instant() = delete;

        inline constexpr Instant operator+(const Duration &rhs) const {
            return Instant{ms_ + rhs.millis()};
        }

        inline constexpr Instant &operator+=(const Duration &rhs) {
            ms_ += rhs.millis();
            return *this;
        }

        inline constexpr Instant operator-(const Duration &rhs) const {
            return Instant{ms_ - rhs.millis()};
        }

        inline constexpr Instant &operator-=(const Duration &rhs) {
            ms_ -= rhs.millis();
            return *this;
        }

        inline constexpr Duration operator-(const Instant &rhs) const {
            return Duration::from_millis(ms_ - rhs.ms_);
        }

        inline constexpr bool operator==(const Instant &rhs) const {
            return ms_ == rhs.ms_;
        }

        inline constexpr bool operator!=(const Instant &rhs) const {
            return ms_ != rhs.ms_;
        }
    };

    class Time {
      public:
        Instant now() const;
    };

    class MockTime {
        Instant now_;

      public:
        MockTime() = delete;

        MockTime(TimeInt now_ms) : now_{now_ms} {}

        Instant now() const;

        void set_now(TimeInt now);

        TimeInt get_now() const;
    };
}; // namespace util
