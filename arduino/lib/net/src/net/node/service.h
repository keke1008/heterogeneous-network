#pragma once

#include "./cost.h"
#include "./source.h"
#include <net/link.h>

namespace net::node {
    struct LocalNodeInfo {
        Cost cost;
        Source source;
    };

    class LocalNodeService {
        nb::Poll<LocalNodeInfo> info_;

      public:
        explicit LocalNodeService() : info_{nb::pending} {}

        inline const nb::Poll<LocalNodeInfo> &poll_info() const {
            return info_;
        }

        template <nb::AsyncReadableWritable RW>
        inline void execute(link::LinkService<RW> &link_service) {
            if (info_.is_ready()) {
                return;
            }

            const etl::optional<link::Address> &opt_self_id = link_service.get_media_address();
            if (!opt_self_id.has_value()) {
                return;
            }

            info_ = LocalNodeInfo{
                .cost = Cost(0),
                .source =
                    Source{
                        .node_id = NodeId{*opt_self_id},
                        .cluster_id_ = OptionalClusterId::no_cluster(),
                    },
            };
        }
    };
} // namespace net::node
