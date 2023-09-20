#pragma once

#include "../../command/cs.h"
#include "util/visitor.h"
#include <etl/variant.h>
#include <nb/poll.h>
#include <nb/time.h>
#include <util/time.h>

namespace net::link::uhf::data_transmisson {
    class RandomBackoffExecutor {
        // 300byte(256byteデータ+適当な制御用byte) / 4.8kbps = 62.5ms
        // 62.5msの前後でランダムにバックオフする
        static inline constexpr util::TimeInt backoff_min_ms = 50;
        static inline constexpr util::TimeInt backoff_max_ms = 100;

        nb::Delay delay_;

        template <typename Rand>
        static inline util::Duration gen_backoff_time(Rand rand) {
            auto backoff_time = rand.gen(backoff_min_ms, backoff_max_ms);
            return util::Duration::from_millis(backoff_time);
        }

      public:
        template <typename Rand>
        explicit RandomBackoffExecutor(util::Time &time, Rand rand)
            : delay_{time, gen_backoff_time(rand)} {}

        inline nb::Poll<void> poll(util::Time &time) {
            return delay_.poll(time);
        }
    };

    class CarrierSenseExecutor {
        etl::variant<CSExecutor, RandomBackoffExecutor> executor_;

      public:
        template <typename Serial, typename Rand>
        nb::Poll<void> poll(Serial &serial, util::Time &time, Rand rand) {
            return etl::visit<nb::Poll<void>>(
                util::Visitor{
                    [&](CSExecutor &executor) -> nb::Poll<void> {
                        bool result = POLL_UNWRAP_OR_RETURN(executor.poll(serial));
                        if (result) {
                            return nb::ready();
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
}; // namespace net::link::uhf::data_transmisson
