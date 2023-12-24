#pragma once

#include "../constants.h"
#include "../frame.h"
#include <tl/vec.h>

namespace net::discovery::task {
    class FrameDelayPool {
        struct FrameDelayEntry {
            DiscoveryFrame frame;
            nb::Delay delay;
        };

        tl::Vec<FrameDelayEntry, FRAME_DELAY_POOL_SIZE> pool_;

      public:
        nb::Poll<void> poll_pushable(util::Time &time) {
            return pool_.full() ? nb::pending : nb::ready();
        }

        void push(DiscoveryFrame &&frame, util::Duration delay, util::Time &time) {
            FASSERT(!pool_.full());
            pool_.emplace_back(etl::move(frame), nb::Delay{time, delay});
        }

        nb::Poll<DiscoveryFrame> poll_pop_expired(util::Time &time) {
            for (uint8_t i = 0; i < pool_.size(); i++) {
                if (pool_[i].delay.poll(time).is_ready()) {
                    return pool_.swap_remove(i).frame;
                }
            }

            return nb::pending;
        }
    };
}; // namespace net::discovery::task
