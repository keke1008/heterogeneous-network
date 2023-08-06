#pragma once

#include "../../command/cs.h"
#include "util/visitor.h"
#include <etl/variant.h>
#include <nb/poll.h>
#include <util/time.h>

namespace media::uhf::data_transmisson {
    class BackoffExecutor {
        util::Instant start_;
        util::Duration duration_;

      public:
        explicit inline BackoffExecutor(util::Instant start, util::Duration duration)
            : start_{start},
              duration_{duration} {}

        template <typename Time>
        inline nb::Poll<nb::Empty> poll(Time &time) {
            if (time.now() - start_ > duration_) {
                return nb::empty;
            } else {
                return nb::pending;
            }
        }
    };

    class RandomBackoffExecutor {
        BackoffExecutor executor_;

        // 300byte(256byteデータ+適当な制御用byte) / 4.8kbps = 62.5ms
        // 62.5msの前後でランダムにバックオフする
        static inline constexpr util::TimeInt backoff_min_ms = 50;
        static inline constexpr util::TimeInt backoff_max_ms = 100;

        template <typename Rand>
        static inline util::Duration gen_backoff_time(Rand rand) {
            auto backoff_time = rand.template gen(backoff_min_ms, backoff_max_ms);
            return util::Duration::from_millis(backoff_time);
        }

      public:
        template <typename Time, typename Rand>
        explicit RandomBackoffExecutor(Time &time, Rand rand)
            : executor_{time.now(), gen_backoff_time(rand)} {}

        template <typename Time>
        inline nb::Poll<nb::Empty> poll(Time &time) {
            return executor_.poll(time);
        }
    };

    class CarrierSenseExecutor {
        etl::variant<CSExecutor, RandomBackoffExecutor> executor_;

      public:
        template <typename Serial, typename Time, typename Rand>
        nb::Poll<nb::Empty> poll(Serial &serial, Time &time, Rand rand) {
            return etl::visit<nb::Poll<nb::Empty>>(
                util::Visitor{
                    [&](CSExecutor &executor) -> nb::Poll<nb::Empty> {
                        bool result = POLL_UNWRAP_OR_RETURN(executor.poll(serial));
                        if (result) {
                            return nb::empty;
                        } else {
                            executor_ = RandomBackoffExecutor{time, rand};
                            return nb::pending;
                        }
                    },
                    [&](RandomBackoffExecutor &executor) {
                        POLL_UNWRAP_OR_RETURN(executor.poll(time));
                        executor_ = CSExecutor{};
                        return nb::pending;
                    },
                },
                executor_
            );
        }
    };
}; // namespace media::uhf::data_transmisson
