#pragma once

#include "./constants.h"
#include "./frame.h"
#include <net/node.h>
#include <tl/cache_set.h>

namespace net::discovery {
    struct CacheValue {
        node::NodeId gateway_id;
        TotalCost total_cost;
    };

    template <typename T>
    struct CacheEntry {
        T destination;
        nb::Delay timeout;
        CacheValue value;
    };

    template <typename T>
    class DiscoveryCacheSet {
        tl::CacheSet<CacheEntry<T>, MAX_DISCOVERY_CACHE_ENTRIES> entries_;

      public:
        inline void update(
            const T &destination,
            const node::NodeId &gateway_id,
            TotalCost total_cost,
            util::Time &time
        ) {
            entries_.remove_once_if([&](const CacheEntry<T> &entry) {
                return entry.destination == destination;
            });

            entries_.push(CacheEntry<T>{
                .destination = destination,
                .timeout = nb::Delay{time, DISCOVERY_CACHE_EXPIRATION},
                .value =
                    CacheValue{
                        .gateway_id = gateway_id,
                        .total_cost = total_cost,
                    },
            });
        }

        inline void remove(const node::NodeId &gateway_id) {
            entries_.remove_once_if([&](const CacheEntry<T> &entry) {
                return entry.value.gateway_id == gateway_id;
            });
        }

        etl::optional<etl::reference_wrapper<const CacheValue>> get(const T &destination) const {
            auto found = entries_.get_if([&](const CacheEntry<T> &e) {
                return e.destination == destination;
            });
            if (found.has_value()) {
                return etl::optional(etl::cref(found->get().value));
            } else {
                return etl::nullopt;
            }
        }

        inline void remove_expired(util::Instant now) {
            entries_.remove_all_if([&](const CacheEntry<T> &entry) {
                return entry.timeout.poll(now).is_ready();
            });
        }
    };

    class DiscoveryCache {
        nb::Debounce remove_expired_debounce_;
        DiscoveryCacheSet<node::NodeId> node_id_entries_;
        DiscoveryCacheSet<node::ClusterId> cluster_id_entries_;

      public:
        explicit DiscoveryCache(util::Time &time)
            : remove_expired_debounce_{time, DISCOVERY_CACHE_EXPIRATION_CHECK_INTERVAL} {}

        void update(
            const node::Destination &destination,
            const node::NodeId &gateway_id,
            TotalCost total_cost,
            util::Time &time
        ) {
            const auto &opt_node_id = destination.node_id;
            if (opt_node_id.has_value()) {
                node_id_entries_.update(*opt_node_id, gateway_id, total_cost, time);
            }

            auto opt_cluster_id = destination.cluster_id;
            if (opt_cluster_id.has_value()) {
                cluster_id_entries_.update(*opt_cluster_id, gateway_id, total_cost, time);
            }
        }

        inline void update_by_received_frame(
            const ReceivedDiscoveryFrame &frame,
            TotalCost total_cost,
            util::Time &time
        ) {
            update(frame.start_node(), frame.sender.node_id, total_cost, time);
        }

        inline void remove(const node::NodeId &gateway_id) {
            node_id_entries_.remove(gateway_id);
            cluster_id_entries_.remove(gateway_id);
        }

        etl::optional<etl::reference_wrapper<const CacheValue>>
        get(const node::Destination &destination) const {
            // まずは NodeId が一致するものを探す
            const auto &opt_node_id = destination.node_id;
            if (opt_node_id.has_value()) {
                auto opt_ref_gateway = node_id_entries_.get(*opt_node_id);
                if (opt_ref_gateway.has_value()) {
                    return opt_ref_gateway;
                }
            }

            // 次に ClusterId が一致するものを探す
            auto opt_cluster_id = destination.cluster_id;
            if (opt_cluster_id.has_value()) {
                auto opt_ref_gateway = cluster_id_entries_.get(*opt_cluster_id);
                if (opt_ref_gateway.has_value()) {
                    return opt_ref_gateway;
                }
            }

            // どちらも一致しない場合は見つからない
            return etl::nullopt;
        }

        etl::optional<etl::reference_wrapper<const CacheValue>> get(const node::NodeId &destination
        ) const {
            return node_id_entries_.get(destination);
        }

        inline void execute(util::Time &time) {
            auto now = time.now();
            node_id_entries_.remove_expired(now);
            cluster_id_entries_.remove_expired(now);
        }
    };
} // namespace net::discovery
