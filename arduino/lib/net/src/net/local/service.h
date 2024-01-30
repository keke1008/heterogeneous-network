#pragma once

#include "./config.h"
#include "./dynamic_cost.h"
#include "./info.h"

namespace net::local {
    class LocalNodeService {
        LocalNodeConfig config_;
        LocalNodeInfoStorage info_;
        DynamicCostUpdator dynamic_cost_;

      public:
        inline explicit LocalNodeService(util::Time &time) : dynamic_cost_{time} {}

        inline void execute(
            link::MediaService auto &ms,
            link::LinkService &ls,
            notification::NotificationService &nts,
            util::Time &time
        ) {
            info_.execute(ms, nts);
            dynamic_cost_.execute(ls, nts, info_, config_, time);
        }

        inline const nb::Poll<LocalNodeInfo> &poll_info() const {
            return info_.get();
        }

        inline const LocalNodeConfig &config() const {
            return config_;
        }

        inline nb::Poll<void> set_cost(notification::NotificationService &nts, node::Cost cost) {
            return info_.set_cost(nts, cost);
        }

        inline nb::Poll<void>
        set_cluster_id(notification::NotificationService &nts, node::OptionalClusterId cluster_id) {
            return info_.set_cluster_id(nts, cluster_id);
        }
    };
} // namespace net::local
