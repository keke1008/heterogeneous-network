#pragma once

#include <nb/time.h>
#include <stdint.h>
#include <util/time.h>

namespace net::link {
    class Measurement {
        uint16_t received_frame_count_{0};
        uint16_t accepted_frame_count_{0};
        util::Duration sum_of_received_frame_wait_time_{util::Duration::zero()};

      public:
        inline void on_frame_received() {
            received_frame_count_++;
        }

        inline void on_frame_accepted(nb::Delay expiration, util::Time &time) {
            accepted_frame_count_++;
            sum_of_received_frame_wait_time_ += time.now() - expiration.start();
        }

        inline uint16_t received_frame_count() const {
            return received_frame_count_;
        }

        inline util::Duration sum_of_received_frame_wait_time() const {
            return sum_of_received_frame_wait_time_;
        }

        inline uint16_t accepted_frame_count() const {
            return accepted_frame_count_;
        }

        inline void reset() {
            received_frame_count_ = 0;
            accepted_frame_count_ = 0;
            sum_of_received_frame_wait_time_ = util::Duration::zero();
        }
    };

} // namespace net::link
