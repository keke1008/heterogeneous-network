#pragma once

#include "../../command/cs.h"
#include "util/visitor.h"
#include <etl/variant.h>
#include <nb/poll.h>
#include <nb/time.h>
#include <util/rand.h>
#include <util/time.h>

namespace net::link::uhf::data_transmisson {
    class RandomBackoffExecutor {
        // 300byte(256byteデータ+適当な制御用byte) / 4.8kbps = 62.5ms
        // 62.5msの前後でランダムにバックオフする
        static inline constexpr util::TimeInt backoff_min_ms = 50;
        static inline constexpr util::TimeInt backoff_max_ms = 100;

        nb::Delay delay_;

        static inline util::Duration gen_backoff_time(util::Rand &rand) {
            auto backoff_time = rand.gen_uint8_t(backoff_min_ms, backoff_max_ms);
            return util::Duration::from_millis(backoff_time);
        }

      public:
        explicit RandomBackoffExecutor(util::Time &time, util::Rand &rand)
            : delay_{time, gen_backoff_time(rand)} {}

        inline nb::Poll<void> poll(util::Time &time) {
            return delay_.poll(time);
        }
    };

    class CarrierSenseExecutor {
        etl::variant<CSExecutor, RandomBackoffExecutor> executor_;

      public:
        nb::Poll<void>
        poll(nb::stream::ReadableWritableStream &stream, util::Time &time, util::Rand &rand) {
            return etl::visit<nb::Poll<void>>(
                util::Visitor{
                    [&](CSExecutor &executor) -> nb::Poll<void> {
                        bool result = POLL_UNWRAP_OR_RETURN(executor.poll(stream));
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
