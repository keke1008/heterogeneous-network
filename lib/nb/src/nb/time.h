#pragma once

#include <nb/poll.h>
#include <util/time.h>

namespace nb {
    class Delay {
        util::Instant start_;
        util::Duration duration_;

      public:
        template <typename Time>
        explicit inline Delay(Time &time, util::Duration duration)
            : start_{time.now()},
              duration_{duration} {}

        template <typename Time>
        inline nb::Poll<nb::Empty> poll(Time &time) {
            if (time.now() - start_ >= duration_) {
                return nb::empty;
            } else {
                return nb::pending;
            }
        }
    };
} // namespace nb
