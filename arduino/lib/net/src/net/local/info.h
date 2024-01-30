#pragma once

#include <net/link.h>
#include <net/node.h>
#include <net/notification.h>

namespace net::local {
    struct LocalNodeInfo {
        node::Cost cost;
        node::Source source;
    };

    class LocalNodeInfoStorage {
        nb::Poll<LocalNodeInfo> info_{nb::pending};

      public:
        inline void execute(link::MediaService auto &ms, notification::NotificationService &nts) {
            if (info_.is_ready()) {
                return;
            }

            const etl::optional<link::Address> &opt_self_id = ms.get_media_address();
            if (!opt_self_id.has_value()) {
                return;
            }

            info_ = LocalNodeInfo{
                .cost = node::Cost(0),
                .source =
                    node::Source{
                        .node_id = node::NodeId{*opt_self_id},
                        .cluster_id = node::OptionalClusterId::no_cluster(),
                    },
            };

            const auto &info = info_.unwrap();
            nts.notify(notification::SelfUpdated{
                .cluster_id = info.source.cluster_id,
                .cost = info.cost,
            });
        }

        inline const nb::Poll<LocalNodeInfo> &get() const {
            return info_;
        }

        inline nb::Poll<void> set_cost(notification::NotificationService &nts, node::Cost cost) {
            LocalNodeInfo prev = POLL_UNWRAP_OR_RETURN(info_);
            info_ = LocalNodeInfo{
                .cost = cost,
                .source = prev.source,
            };
            nts.notify(notification::SelfUpdated{
                .cluster_id = prev.source.cluster_id,
                .cost = cost,
            });
            return nb::ready();
        }

        inline nb::Poll<void>
        set_cluster_id(notification::NotificationService &nts, node::OptionalClusterId cluster_id) {
            LocalNodeInfo prev = POLL_UNWRAP_OR_RETURN(info_);
            info_ = LocalNodeInfo{
                .cost = prev.cost,
                .source = node::Source{.node_id = prev.source.node_id, .cluster_id = cluster_id}
            };
            nts.notify(notification::SelfUpdated{
                .cluster_id = cluster_id,
                .cost = prev.cost,
            });
            return nb::ready();
        }
    };
} // namespace net::local
