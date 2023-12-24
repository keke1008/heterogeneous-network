#pragma once

#include <net/link.h>
#include <net/node.h>

namespace net::local {
    struct LocalNodeInfo {
        node::Cost cost;
        node::Source source;
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
                .cost = node::Cost(0),
                .source =
                    node::Source{
                        .node_id = node::NodeId{*opt_self_id},
                        .cluster_id = node::OptionalClusterId::no_cluster(),
                    },
            };
        }
    };
} // namespace net::local
