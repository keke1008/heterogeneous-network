#pragma once

#include <stdint.h>

namespace util {
    using TimeInt = uint32_t;
    using TimeDiff = int32_t;

    class Duration {
        TimeDiff ms_;

        inline constexpr Duration(TimeDiff ms) : ms_{ms} {}

      public:
        Duration() = delete;

        static inline constexpr Duration from_millis(TimeDiff ms) {
            return Duration{ms};
        }

        static inline constexpr Duration from_seconds(TimeDiff s) {
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

        inline constexpr TimeDiff millis() const {
            return ms_;
        }

        inline constexpr TimeDiff seconds() const {
            return ms_ / 1000;
        }
    };

    class Instant {
        friend class Time;
        friend class MockTime;
        friend class ArduinoTime;

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

        inline constexpr bool operator<(const Instant &rhs) const {
            return ms_ < rhs.ms_;
        }

        inline constexpr bool operator<=(const Instant &rhs) const {
            return ms_ <= rhs.ms_;
        }

        inline constexpr bool operator>(const Instant &rhs) const {
            return ms_ > rhs.ms_;
        }

        inline constexpr bool operator>=(const Instant &rhs) const {
            return ms_ >= rhs.ms_;
        }
    };

    class Time {
      public:
        virtual Instant now() const = 0;
    };

    class MockTime final : public Time {
        Instant now_;

      public:
        MockTime() = delete;

        MockTime(TimeInt now_ms) : now_{now_ms} {}

        Instant now() const override {
            return now_;
        }

        inline void set_now_ms(TimeInt now) {
            now_ = Instant{now};
        }

        inline TimeInt get_now_ms() const {
            return now_.ms_;
        }

        inline void advance(Duration duration) {
            now_ += duration;
        }
    };

    class ArduinoTime final : public Time {
      public:
        inline Instant now() const override;
    };
}; // namespace util

#if __has_include(<Arduino.h>)

#include <Arduino.h>

namespace util {
    Instant ArduinoTime::now() const {
        return Instant{millis()};
    }
}; // namespace util

#endif
