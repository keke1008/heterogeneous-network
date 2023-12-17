#pragma once

#include "./constants.h"
#include "./frame.h"
#include <net/node.h>
#include <tl/cache_set.h>

namespace net::discovery {
    template <typename T>
    struct CacheEntry {
        T destination;
        node::NodeId gateway_id;
    };

    template <typename T>
    class DiscoveryCacheSet {
        tl::CacheSet<CacheEntry<T>, MAX_DISCOVERY_CACHE_ENTRIES> entries_;

      public:
        inline void update(const T &destination, const node::NodeId &gateway_id) {
            entries_.remove_once_if([&](const CacheEntry<T> &entry) {
                return entry.destination == destination;
            });

            entries_.push(CacheEntry<T>{
                .destination = destination,
                .gateway_id = gateway_id,
            });
        }

        inline void remove(const node::NodeId &gateway_id) {
            entries_.remove_once_if([&](const CacheEntry<T> &entry) {
                return entry.gateway_id == gateway_id;
            });
        }

        etl::optional<etl::reference_wrapper<const node::NodeId>> get(const T &destination) const {
            auto found = entries_.get_if([&](const CacheEntry<T> &e) {
                return e.destination == destination;
            });
            if (found.has_value()) {
                return etl::optional(etl::cref(found->get().gateway_id));
            } else {
                return etl::nullopt;
            }
        }
    };

    class DiscoveryCache {
        DiscoveryCacheSet<node::NodeId> node_id_entries_;
        DiscoveryCacheSet<node::ClusterId> cluster_id_entries_;

      public:
        void update(const node::Destination &destination, const node::NodeId &gateway_id) {
            const auto &opt_node_id = destination.node_id;
            if (opt_node_id.has_value()) {
                node_id_entries_.update(*opt_node_id, gateway_id);
            }

            auto opt_cluster_id = destination.cluster_id;
            if (opt_cluster_id.has_value()) {
                cluster_id_entries_.update(*opt_cluster_id, gateway_id);
            }
        }

        inline void update_by_received_frame(const DiscoveryFrame &frame) {
            update(frame.start_node(), frame.sender.node_id);
        }

        inline void remove(const node::NodeId &gateway_id) {
            node_id_entries_.remove(gateway_id);
            cluster_id_entries_.remove(gateway_id);
        }

        etl::optional<etl::reference_wrapper<const node::NodeId>>
        get(const node::Destination &destination) const {
            // まずは NodeId が一致するものを探す
            const auto &opt_node_id = destination.node_id;
            if (opt_node_id.has_value()) {
                return node_id_entries_.get(*opt_node_id);
            }

            // 次に ClusterId が一致するものを探す
            auto opt_cluster_id = destination.cluster_id;
            if (opt_cluster_id.has_value()) {
                return cluster_id_entries_.get(*opt_cluster_id);
            }

            // どちらも一致しない場合は見つからない
            return etl::nullopt;
        }

        etl::optional<etl::reference_wrapper<const node::NodeId>>
        get(const node::NodeId &destination) const {
            return node_id_entries_.get(destination);
        }
    };
} // namespace net::discovery
