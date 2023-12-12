#pragma once

#include "./constants.h"
#include <net/node.h>
#include <tl/cache_set.h>

namespace net::discovery {
    struct DiscoveryCacheEntry {
        node::NodeId target_id;
        node::NodeId gateway_id;
    };

    class DiscoveryCache {
        tl::CacheSet<DiscoveryCacheEntry, MAX_DISCOVERY_CACHE_ENTRIES> entries_;

        inline etl::optional<uint8_t> find_index_by_target_id(const node::NodeId &target_id) const {
            return entries_.find_index([&target_id](const DiscoveryCacheEntry &entry) {
                return entry.target_id == target_id;
            });
        }

        inline etl::optional<uint8_t> find_index_by_gateway_id(const node::NodeId &gateway_id
        ) const {
            return entries_.find_index([&gateway_id](const DiscoveryCacheEntry &entry) {
                return entry.gateway_id == gateway_id;
            });
        }

      public:
        inline void add(const node::NodeId &target_id, const node::NodeId &gateway_id) {
            if (!find_index_by_target_id(target_id)) {
                entries_.push(DiscoveryCacheEntry{target_id, gateway_id});
            }
        }

        inline void remove(const node::NodeId &gateway_id) {
            auto index = find_index_by_gateway_id(gateway_id);
            if (index) {
                entries_.remove(*index);
            }
        }

        etl::optional<etl::reference_wrapper<const node::NodeId>> get(const node::NodeId &target_id
        ) const {
            auto index = find_index_by_target_id(target_id);
            if (index) {
                return etl::cref(entries_.get(*index)->get().gateway_id);
            } else {
                return etl::nullopt;
            }
        }
    };
} // namespace net::discovery
