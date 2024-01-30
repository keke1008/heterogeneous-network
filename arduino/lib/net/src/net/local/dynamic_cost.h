#pragma once

#include "./constants.h"
#include "./info.h"
#include <nb/time.h>
#include <net/node.h>

namespace net::local {
    class DynamicCostUpdator {
        nb::Debounce update_debounce_;

      public:
        explicit DynamicCostUpdator(util::Time &time)
            : update_debounce_{time, DYNAMIC_COST_UPDATE_INTERVAL} {}

        inline void execute(
            link::LinkService &ls,
            notification::NotificationService &nts,
            LocalNodeInfoStorage &info,
            util::Time &time
        ) {
            if (update_debounce_.poll(time).is_pending()) {
                return;
            }

            auto &measurements = ls.measurement();

            // 待ち行列理論に基づいたコスト計算
            float lambda = static_cast<float>(measurements.received_frame_count()) /
                DYNAMIC_COST_UPDATE_INTERVAL.millis();
            float ts = measurements.average_received_frame_wait_time().millis();
            float rho = lambda * ts;
            float tw = rho / (1 - rho) * ts;

            auto cost = util::Duration::from_millis(static_cast<util::TimeDiff>(tw));
            info.set_cost(nts, static_cast<node::Cost>(cost));

            measurements.reset();
        }
    };
} // namespace net::local
