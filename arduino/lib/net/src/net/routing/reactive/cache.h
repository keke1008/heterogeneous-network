#pragma once

#include "../node.h"
#include "./constants.h"
#include <data/cache_set.h>

namespace net::routing::reactive {
    struct RouteCacheEntry {
        NodeId target_id;
        NodeId gateway_id;
    };

    class RouteCache {
        data::CacheSet<RouteCacheEntry, MAX_ROUTE_CACHE_ENTRIES> entries_;

        inline etl::optional<uint8_t> find_index_by_target_id(const NodeId &target_id) const {
            return entries_.find_index([&target_id](const RouteCacheEntry &entry) {
                return entry.target_id == target_id;
            });
        }

        inline etl::optional<uint8_t> find_index_by_gateway_id(const NodeId &gateway_id) const {
            return entries_.find_index([&gateway_id](const RouteCacheEntry &entry) {
                return entry.gateway_id == gateway_id;
            });
        }

      public:
        inline void add(const NodeId &target_id, const NodeId &gateway_id) {
            if (!find_index_by_target_id(target_id)) {
                entries_.push(RouteCacheEntry{target_id, gateway_id});
            }
        }

        inline void remove(const NodeId &gateway_id) {
            auto index = find_index_by_gateway_id(gateway_id);
            if (index) {
                entries_.remove(*index);
            }
        }

        etl::optional<etl::reference_wrapper<const NodeId>> get(const NodeId &target_id) const {
            auto index = find_index_by_target_id(target_id);
            if (index) {
                return etl::cref(entries_.get(*index)->get().gateway_id);
            } else {
                return etl::nullopt;
            }
        }
    };
} // namespace net::routing::reactive
