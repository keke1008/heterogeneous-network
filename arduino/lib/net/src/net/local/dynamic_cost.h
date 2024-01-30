#pragma once

#include "./config.h"
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
            LocalNodeConfig config,
            util::Time &time
        ) {
            if (update_debounce_.poll(time).is_pending()) {
                return;
            }

            auto &measurements = ls.measurement();

            if (config.enable_dynamic_cost_update) {
                // 待ち行列理論に基づいたコスト計算を行う
                float lambda = static_cast<float>(measurements.received_frame_count()) /
                    DYNAMIC_COST_UPDATE_INTERVAL.millis();
                float ts =
                    static_cast<float>(measurements.sum_of_received_frame_wait_time().millis()) /
                    measurements.accepted_frame_count();
                float rho = lambda * ts;
                float tw = rho / (1 - rho) * ts;
                auto cost = util::Duration::from_millis(static_cast<util::TimeDiff>(tw));
                info.set_cost(nts, static_cast<node::Cost>(cost));
            }

            measurements.reset();
        }
    };
} // namespace net::local
