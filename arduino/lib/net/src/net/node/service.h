#pragma once

#include "./cluster_id.h"
#include "./cost.h"
#include "./node_id.h"
#include <net/link.h>

namespace net::node {
    struct LocalNodeInfo {
        NodeId id;
        Cost cost;
        ClusterId cluster_id;
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
                .id = NodeId(opt_self_id.value()),
                .cost = Cost(0),
                .cluster_id = ClusterId::default_id(),
            };
        }
    };
} // namespace net::node
