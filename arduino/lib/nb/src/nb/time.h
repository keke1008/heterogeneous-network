#pragma once

#include <nb/poll.h>
#include <util/time.h>

namespace nb {
    class Delay {
        util::Instant start_;
        util::Duration duration_;

      public:
        explicit inline Delay(util::Time &time, util::Duration duration)
            : start_{time.now()},
              duration_{duration} {}

        inline nb::Poll<void> poll(util::Time &time) const {
            if (time.now() - start_ >= duration_) {
                return nb::ready();
            } else {
                return nb::pending;
            }
        }

        inline void set_duration(util::Duration duration) {
            duration_ = duration;
        }
    };

    class Debounce {
        util::Instant last_;
        util::Duration duration_;

      public:
        explicit inline Debounce(util::Time &time, util::Duration duration)
            : last_{time.now()},
              duration_{duration} {}

        inline nb::Poll<void> poll(util::Time &time) {
            util::Instant now = time.now();
            if (now - last_ >= duration_) {
                last_ = now;
                return nb::ready();
            } else {
                return nb::pending;
            }
        }
    };
} // namespace nb
